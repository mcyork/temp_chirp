/*
 * NTP Clock - Firmware v2.17
 * Hardware: ESP32-S3-MINI-1
 * 
 * Features:
 * - WiFi connection to NTP server
 * - Displays time as HHMM on 7-segment display
 * - Decimal point on 100s digit flashes like a colon (seconds indicator)
 * - AP mode web interface for WiFi and preferences configuration
 * - Timezone configuration via Preferences with automatic DST handling
 * - Version display at boot
 * - IP address scrolling in AP mode
 * - Improv WiFi provisioning via ESP Web Tools
 */

 #define FIRMWARE_VERSION "2.17"

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <SPI.h>
#include <Preferences.h>
#include <string.h>
#include <ImprovWiFiLibrary.h>
#include "SevenSegmentDisplay/MAX7219Display.h"
#include "SevenSegmentDisplay/MAX7219Display.cpp"
#include "web_pages.h"

// =============================================================================
// ESP32-S3 USB CDC WORKAROUND
// ESP32-S3 has a bug where Serial.available() doesn't reliably update.
// This buffered wrapper uses event callbacks to capture incoming data.
// =============================================================================
class BufferedHWCDC : public Stream {
private:
  uint8_t buffer[256];
  volatile uint16_t head = 0;
  volatile uint16_t tail = 0;
  volatile uint16_t count = 0;

public:
  void feedByte(uint8_t byte) {
    if (count < sizeof(buffer)) {
      buffer[tail] = byte;
      tail = (tail + 1) % sizeof(buffer);
      count++;
    }
  }

  virtual int available() override {
    return count;
  }

  virtual int read() override {
    if (count == 0) return -1;
    uint8_t byte = buffer[head];
    head = (head + 1) % sizeof(buffer);
    count--;
    return byte;
  }

  virtual int peek() override {
    if (count == 0) return -1;
    return buffer[head];
  }

  virtual void flush() override {
    // Nothing to flush for input buffer
  }

  virtual size_t write(uint8_t byte) override {
    return Serial.write(byte);
  }

  virtual size_t write(const uint8_t *buffer, size_t size) override {
    return Serial.write(buffer, size);
  }

  virtual int availableForWrite() override {
    return Serial.availableForWrite();
  }
};

BufferedHWCDC bufferedSerial;

// Event callback for ESP32-S3 USB CDC RX events
// This is called by Serial.onEvent() when data arrives
static void hwcdcEventCallback(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data) {
  if (event_base == ARDUINO_HW_CDC_EVENTS && event_id == ARDUINO_HW_CDC_RX_EVENT) {
    // Read all available bytes from Serial and feed to buffer
    int count = 0;
    while (Serial.available()) {
      bufferedSerial.feedByte(Serial.read());
      count++;
    }
    if (count > 0) {
      Serial.printf("[EVENT] Fed %d bytes to buffer\n", count);
      Serial.flush();
    }
  }
}
 
 // --- PIN DEFINITIONS ---
 const int PIN_BTN_MODE = 7; 
 const int PIN_BTN_UP   = 6; 
 const int PIN_BTN_DOWN = 5; 
 const int PIN_BUZZER   = 4; 
 
 // SPI Bus
 const int PIN_SPI_SCK  = 16;
 const int PIN_SPI_MOSI = 17;
 const int PIN_SPI_MISO = 18;
 
// Chip Selects
const int PIN_CS_DISP  = 11;
 
 // --- AP MODE CONFIGURATION ---
 String apSSID = "";  // Will be set dynamically using MAC address
 const char* AP_PASSWORD = ""; // Open AP
 
 // --- NTP CONFIGURATION ---
 const char* ntpServer = "pool.ntp.org";
 long  gmtOffset_sec = 0;  // Will be loaded from Preferences
 int   daylightOffset_sec = 3600;  // Default 1 hour for DST
 
// --- OBJECTS ---
MAX7219Display display(PIN_CS_DISP);
Preferences preferences;
WebServer server(80);
// Use buffered wrapper instead of Serial directly to work around ESP32-S3 USB CDC bug
ImprovWiFi improvSerial(&bufferedSerial);
 
// --- STATE VARIABLES ---
bool wifiConnected = false;
bool timeSynced = false;
bool apMode = false;
unsigned long lastTimeUpdate = 0;
int displayBrightness = 8; // 0-15, default medium
bool use24Hour = true; // 24-hour format (true) or 12-hour format (false)
bool showIPAddress = false; // Flag to show IP address twice after WiFi connects
int ipDisplayCount = 0; // Count how many times IP has been displayed
bool showingVersion = true; // Guard flag to prevent loop() from overwriting version display
unsigned long versionStartTime = 0; // When version display started
bool showConnAfterVersion = false; // Flag to show "Conn" after version display
bool showAPAfterVersion = false; // Flag to show "AP" after version display
bool improvConnected = false; // Flag set when Improv WiFi successfully connects

// Beep state variables (using LEDC instead of tone() to avoid timer conflicts)
unsigned long beepEndTime = 0;
bool beepActive = false;
 
 // Button state tracking
 unsigned long lastButtonPress = 0;
 bool lastModeState = false;
 bool lastUpState = false;
 bool lastDownState = false;
 
 // Forward declarations
 void handleButtons();
void handleRoot();
void handleConfig();
void handleSave();
void handleFactoryReset();
void startBeep(int frequency, int duration);
void updateBeep();
void beepBlocking(int frequency, int duration);
bool detectTimezoneFromIP();

// =============================================================================
// IMPROV WIFI CALLBACKS - These are REQUIRED for Improv to work!
// =============================================================================

// This function is called by the Improv library to actually connect to WiFi
// WITHOUT THIS, Improv receives credentials but doesn't know how to use them!
bool onImprovWiFiConnect(const char* ssid, const char* password) {
  Serial.printf("Improv: Attempting to connect to SSID: %s\n", ssid);
  Serial.flush();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Wait for connection (up to 10 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Improv: Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.flush();
    improvConnected = true;
    return true;
  } else {
    Serial.println("Improv: Connection failed");
    Serial.flush();
    return false;
  }
}

// This callback is called AFTER successful WiFi connection via Improv
// Use this to save credentials and do post-connection setup
void onImprovWiFiConnected(const char* ssid, const char* password) {
  Serial.printf("Improv: Saving credentials for SSID: %s\n", ssid);
  
  // Save WiFi credentials
  preferences.begin("wifi_config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  
  // Try to auto-detect timezone from IP geolocation if not already configured
  preferences.begin("ntp_clock", false);
  long savedTimezone = preferences.getLong("timezone", 0);
  preferences.end();
  
  // Only auto-detect if timezone hasn't been manually configured (default is 0)
  if (savedTimezone == 0) {
    detectTimezoneFromIP();
  }
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
  // Init Serial IMMEDIATELY - ESP32-S3 USB CDC needs this early
  Serial.begin(115200);
  
  // CRITICAL: Wait for USB CDC to enumerate on ESP32-S3
  // This is necessary because USB CDC takes time to initialize after reboot
  unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 2000) {
    delay(10);
  }
  delay(100); // Extra settle time
  
  // CRITICAL: Register event callback for ESP32-S3 USB CDC workaround
  // This captures incoming Serial data via events since Serial.available() is broken
  // With USB CDC on boot enabled (USBMode=hwcdc,CDCOnBoot=1 in build.yml), Serial is HWCDC
  Serial.onEvent(hwcdcEventCallback);
  
  Serial.println("\n\n=== NTP Clock v" FIRMWARE_VERSION " Starting ===");
  Serial.println("HWCDC event handler registered");
  Serial.flush();
  
  // Initialize WiFi in STA mode early - Improv needs this
  WiFi.mode(WIFI_STA);
  delay(100);
  
  // Generate unique AP SSID using MAC address
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  String macSuffix = macAddress.substring(macAddress.length() - 6);
  apSSID = "NTP_Clock_" + macSuffix;
  
  // ==========================================================================
  // IMPROV WIFI SETUP - Must happen BEFORE any blocking operations
  // ==========================================================================
  Serial.println("Setting up Improv WiFi...");
  
  // Configure device info
  improvSerial.setDeviceInfo(
    ImprovTypes::ChipFamily::CF_ESP32_S3,
    "NTP-Clock",           // Short device name
    FIRMWARE_VERSION,      // Firmware version
    "NTP Clock"            // Device description
  );
  
  // THIS IS THE KEY PART YOU WERE MISSING!
  // Set the callback that actually performs the WiFi connection
  improvSerial.setCustomConnectWiFi(onImprovWiFiConnect);
  
  // Set the callback for after successful connection (to save credentials)
  improvSerial.onImprovConnected(onImprovWiFiConnected);
  
  Serial.println("Improv WiFi ready - waiting for provisioning...");
  Serial.flush();
  
  // ==========================================================================
  // IMPROV GRACE PERIOD
  // ESP Web Tools needs time after device reboot to:
  // 1. Detect the device is back
  // 2. Open serial port
  // 3. Send Improv handshake
  // 4. Send credentials
  // ==========================================================================
  unsigned long improvStart = millis();
  unsigned long improvTimeout = 10000; // 10 seconds
  
  while (millis() - improvStart < improvTimeout) {
    // Process Improv commands
    // The bufferedSerial wrapper now handles Serial input via event callbacks
    improvSerial.handleSerial();
    
    // If Improv connected us, we're done waiting
    if (improvConnected || WiFi.status() == WL_CONNECTED) {
      Serial.println("Improv WiFi connected during grace period!");
      break;
    }
    
    // Small delay to prevent tight loop, but stay responsive
    delay(10);
  }
  
  Serial.println("Improv grace period ended");
  Serial.flush();
  
  // ==========================================================================
  // HARDWARE INITIALIZATION
  // ==========================================================================
  
  // Init Pins
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_CS_DISP, OUTPUT);
  digitalWrite(PIN_CS_DISP, HIGH);
  
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  // Initialize display
  display.begin();
  delay(200);
  
  // Load preferences
  preferences.begin("ntp_clock", false);
  displayBrightness = preferences.getInt("brightness", 8);
  use24Hour = preferences.getBool("24hour", true);
  gmtOffset_sec = preferences.getLong("timezone", -28800);
  daylightOffset_sec = preferences.getInt("dst_offset", 0);
  preferences.end();
  
  display.setBrightness(displayBrightness);
  delay(50);
  
  // Show version
  showingVersion = true;
  versionStartTime = millis();
  display.displayText(FIRMWARE_VERSION, true);
  delay(100);
  
  // ==========================================================================
  // WIFI CONNECTION (if not already connected via Improv)
  // ==========================================================================
  
  if (WiFi.status() == WL_CONNECTED) {
    // Already connected via Improv
    Serial.println("Using Improv WiFi connection");
    wifiConnected = true;
    showConnAfterVersion = true;
    beepBlocking(2000, 100);
    
    // Setup web server
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/factory-reset", HTTP_POST, handleFactoryReset);
    server.begin();
    
    showIPAddress = true;
    ipDisplayCount = 0;
    lastTimeUpdate = 0;
    
    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(1000);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      timeSynced = true;
      beepBlocking(2500, 50);
      delay(100);
      beepBlocking(3000, 50);
    }
  } else {
    // Try saved credentials
    preferences.begin("wifi_config", false);
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();
    
    if (savedSSID.length() > 0) {
      Serial.printf("Trying saved credentials: %s\n", savedSSID.c_str());
      WiFi.mode(WIFI_STA);
      WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
      int wifiAttempts = 0;
      while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
        delay(500);
        improvSerial.handleSerial(); // Continue processing Improv during wait
        wifiAttempts++;
      }
    
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to saved WiFi");
        wifiConnected = true;
        showConnAfterVersion = true;
        beepBlocking(2000, 100);
        
        // Try to auto-detect timezone if not configured
        preferences.begin("ntp_clock", false);
        long savedTimezone = preferences.getLong("timezone", 0);
        preferences.end();
        
        if (savedTimezone == 0) {
          detectTimezoneFromIP();
          preferences.begin("ntp_clock", false);
          gmtOffset_sec = preferences.getLong("timezone", -28800);
          daylightOffset_sec = preferences.getInt("dst_offset", 0);
          preferences.end();
        }
        
        // Setup web server
        server.on("/", handleRoot);
        server.on("/config", handleConfig);
        server.on("/save", HTTP_POST, handleSave);
        server.on("/factory-reset", HTTP_POST, handleFactoryReset);
        server.begin();
        
        showIPAddress = true;
        ipDisplayCount = 0;
        lastTimeUpdate = 0;
        
        // Configure NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        delay(1000);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          timeSynced = true;
          beepBlocking(2500, 50);
          delay(100);
          beepBlocking(3000, 50);
        }
      }
    }
  }
  
  // If still not connected, start AP mode
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Starting AP mode");
    apMode = true;
    showAPAfterVersion = true;
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), AP_PASSWORD);
    delay(500);
    
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/factory-reset", HTTP_POST, handleFactoryReset);
    server.begin();
    
    beepBlocking(1500, 200);
  }
  
  display.clear();
  delay(500);
}

// =============================================================================
// LOOP
// =============================================================================

void loop() {
  // FALLBACK: Always poll Serial directly as backup
  // This is a workaround for ESP32-S3 USB CDC Serial.available() bug
  // Even with event handler, polling ensures we don't miss data
  static unsigned long lastPollDebug = 0;
  if (Serial.available() > 0) {
    int pollCount = 0;
    while (Serial.available()) {
      bufferedSerial.feedByte(Serial.read());
      pollCount++;
    }
    if (millis() - lastPollDebug > 1000) {  // Debug once per second max
      Serial.printf("[POLL] Fed %d bytes to buffer\n", pollCount);
      Serial.flush();
      lastPollDebug = millis();
    }
  }
  
  // ALWAYS process Improv commands - allows re-provisioning while running
  improvSerial.handleSerial();
  
  if (showingVersion) {
    if (millis() - versionStartTime >= 5000) {
      showingVersion = false;
      display.clear();
      
      if (showConnAfterVersion) {
        display.displayText("Conn");
        delay(1000);
        showConnAfterVersion = false;
      } else if (showAPAfterVersion) {
        display.displayText("AP  ");
        delay(1000);
        showAPAfterVersion = false;
      }
    }
    return;
  }
  
  updateBeep();
  display.update();
  
  if (apMode) {
    // Check if WiFi connected while in AP mode (e.g., via Improv WiFi provisioning)
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected while in AP mode - switching to STA mode");
      apMode = false;
      wifiConnected = true;
      showIPAddress = true;
      ipDisplayCount = 0;
      display.clear();
      delay(500);
      
      // Configure NTP now that we're connected
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      timeSynced = false;
      lastTimeUpdate = 0;
      
      // Stop AP mode
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      
      beepBlocking(2000, 100);
      delay(100);
      beepBlocking(3000, 100);
      
      // Continue to normal WiFi mode handling below
    } else {
      // Still in AP mode - handle AP mode display
      server.handleClient();
      handleButtons();
      
      static String ipAddressStr = "";
      static bool ipScrollingStarted = false;
      if (!ipScrollingStarted) {
        ipAddressStr = WiFi.softAPIP().toString();
        display.startScrolling(ipAddressStr.c_str(), 350);
        ipScrollingStarted = true;
      }
      return; // Stay in AP mode handling
    }
  }
  
  // Normal WiFi mode (not AP mode)
  if (!apMode) {
    server.handleClient();
    handleButtons();
    
    if (showIPAddress && wifiConnected) {
      static bool ipScrollingStarted = false;
      static unsigned long ipStartTime = 0;
      
      if (!ipScrollingStarted) {
        IPAddress ip = WiFi.localIP();
        char ipStr[16];
        sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        display.startScrolling(ipStr, 350);
        ipScrollingStarted = true;
        ipStartTime = millis();
        ipDisplayCount = 0;
      }
      
      unsigned long elapsed = millis() - ipStartTime;
      if (elapsed > 13000) {
        showIPAddress = false;
        ipDisplayCount = 0;
        ipScrollingStarted = false;
        display.clear();
        delay(500);
      }
      return;
    }
    
    if (millis() - lastTimeUpdate >= 1000) {
      lastTimeUpdate = millis();
      
      if (wifiConnected && timeSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          int hours = timeinfo.tm_hour;
          int minutes = timeinfo.tm_min;
          int seconds = timeinfo.tm_sec;
          bool showColon = (seconds % 2 == 0);
          display.displayTime(hours, minutes, showColon, !use24Hour);
        } else {
          display.displayText("Err ");
          timeSynced = false;
        }
      }
    }
  }
  
  delay(10);
}

// =============================================================================
// BUTTON HANDLERS
// =============================================================================

void handleButtons() {
  bool btnMode = (digitalRead(PIN_BTN_MODE) == LOW);
  bool btnUp = (digitalRead(PIN_BTN_UP) == LOW);
  bool btnDown = (digitalRead(PIN_BTN_DOWN) == LOW);
  unsigned long now = millis();
  
  if (btnMode && !lastModeState && (now - lastButtonPress > 200)) {
    use24Hour = !use24Hour;
    preferences.begin("ntp_clock", false);
    preferences.putBool("24hour", use24Hour);
    preferences.end();
    startBeep(2000, 50);
    lastButtonPress = now;
  }
  lastModeState = btnMode;
  
  if (btnUp && !lastUpState && (now - lastButtonPress > 200)) {
    if (displayBrightness < 15) {
      displayBrightness++;
      display.setBrightness(displayBrightness);
      preferences.begin("ntp_clock", false);
      preferences.putInt("brightness", displayBrightness);
      preferences.end();
      startBeep(1500, 30);
    }
    lastButtonPress = now;
  }
  lastUpState = btnUp;
  
  if (btnDown && !lastDownState && (now - lastButtonPress > 200)) {
    if (displayBrightness > 0) {
      displayBrightness--;
      display.setBrightness(displayBrightness);
      preferences.begin("ntp_clock", false);
      preferences.putInt("brightness", displayBrightness);
      preferences.end();
      startBeep(1000, 30);
    }
    lastButtonPress = now;
  }
  lastDownState = btnDown;
}

// =============================================================================
// WEB SERVER HANDLERS
// =============================================================================

void handleRoot() {
  server.send(200, "text/html", getConfigPageHTML(preferences));
}

void handleConfig() {
  server.send(200, "text/html", getConfigPageHTML(preferences));
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String timezoneStr = server.arg("timezone");
  String dstOffsetStr = server.arg("dst_offset");
  String brightnessStr = server.arg("brightness");
  String hourFormatStr = server.arg("hour_format");
  
  preferences.begin("wifi_config", false);
  preferences.putString("ssid", ssid);
  if (password.length() > 0) {
    preferences.putString("password", password);
  }
  preferences.end();
  
  long timezoneOffset = timezoneStr.toInt();
  int dstOffset = dstOffsetStr.toInt();
  preferences.begin("ntp_clock", false);
  preferences.putLong("timezone", timezoneOffset);
  preferences.putInt("dst_offset", dstOffset);
  
  if (brightnessStr.length() > 0) {
    int brightness = brightnessStr.toInt();
    if (brightness >= 0 && brightness <= 15) {
      preferences.putInt("brightness", brightness);
      displayBrightness = brightness;
      display.setBrightness(displayBrightness);
    }
  }
  
  if (hourFormatStr.length() > 0) {
    preferences.putBool("24hour", hourFormatStr == "24");
    use24Hour = (hourFormatStr == "24");
  }
  
  preferences.end();
  
  server.send(200, "text/html", getSaveSuccessPageHTML());
  delay(1000);
  ESP.restart();
}

void handleFactoryReset() {
  preferences.begin("wifi_config", false);
  preferences.clear();
  preferences.end();
  
  preferences.begin("ntp_clock", false);
  preferences.clear();
  preferences.end();
  
  server.send(200, "text/html", getFactoryResetPageHTML());
  delay(1000);
  ESP.restart();
}

// =============================================================================
// BEEP FUNCTIONS
// =============================================================================

void startBeep(int frequency, int duration) {
  ledcAttach(PIN_BUZZER, frequency, 8);
  ledcWrite(PIN_BUZZER, 128);
  beepEndTime = millis() + duration;
  beepActive = true;
}

void updateBeep() {
  if (beepActive && millis() >= beepEndTime) {
    ledcWrite(PIN_BUZZER, 0);
    ledcDetach(PIN_BUZZER);
    beepActive = false;
  }
}

void beepBlocking(int frequency, int duration) {
  ledcAttach(PIN_BUZZER, frequency, 8);
  ledcWrite(PIN_BUZZER, 128);
  delay(duration);
  ledcWrite(PIN_BUZZER, 0);
  ledcDetach(PIN_BUZZER);
}

// =============================================================================
// TIMEZONE DETECTION
// =============================================================================

bool detectTimezoneFromIP() {
  HTTPClient http;
  http.begin("http://ip-api.com/json/?fields=status,offset");
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error && doc["status"] == "success" && doc.containsKey("offset")) {
      long offset = doc["offset"].as<long>();
      
      preferences.begin("ntp_clock", false);
      preferences.putLong("timezone", offset);
      preferences.putInt("dst_offset", 0);
      preferences.end();
      
      gmtOffset_sec = offset;
      daylightOffset_sec = 0;
      
      http.end();
      return true;
    }
  }
  
  http.end();
  return false;
}
