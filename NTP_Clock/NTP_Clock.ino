/*
 * NTP Clock - Firmware v2.15
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
 */

 #define FIRMWARE_VERSION "2.15"

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <SPI.h>
#include <Preferences.h>
#include <string.h>
#include "SevenSegmentDisplay/MAX7219Display.h"
#include "SevenSegmentDisplay/MAX7219Display.cpp"
#include "web_pages.h"
 
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
 
 // --- DISPLAY LIBRARY ---
 // Display driver is now in SevenSegmentDisplay library
 
 // --- OBJECTS ---
 MAX7219Display display(PIN_CS_DISP);
 Preferences preferences;
 WebServer server(80);
 
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
 
void setup() {
  delay(2000);
  
  // Init Serial for configuration commands from web installer
  Serial.begin(115200);
  delay(1000);  // Give Serial time to stabilize after reboot
  
  // Check for configuration commands from web installer (sent via Serial)
  // Format: CONFIG:SSID=xxx:PASSWORD=yyy:TZ=-28800:DST=0
  // Wait up to 8 seconds for configuration (device reboots after install, then installer needs time)
  String configCmd = "";
  bool configReceived = false;
  unsigned long configStart = millis();
  while (millis() - configStart < 8000 && !configReceived) {
    if (Serial.available()) {
      // Read available bytes and accumulate
      while (Serial.available() > 0 && !configReceived) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
          configCmd.trim();
          if (configCmd.startsWith("CONFIG:")) {
            // Parse configuration
            configCmd = configCmd.substring(7); // Remove "CONFIG:"
            String ssid = "";
            String password = "";
            long timezone = -28800;
            int dstOffset = 0;
            
            int startPos = 0;
            while (startPos < configCmd.length()) {
              int eqPos = configCmd.indexOf('=', startPos);
              int colonPos = configCmd.indexOf(':', startPos);
              if (colonPos == -1) colonPos = configCmd.length();
              
              String key = configCmd.substring(startPos, eqPos);
              String value = configCmd.substring(eqPos + 1, colonPos);
              
              if (key == "SSID") ssid = value;
              else if (key == "PASSWORD") password = value;
              else if (key == "TZ") timezone = value.toInt();
              else if (key == "DST") dstOffset = value.toInt();
              
              startPos = colonPos + 1;
            }
            
            // Save configuration
            if (ssid.length() > 0) {
              preferences.begin("wifi_config", false);
              preferences.putString("ssid", ssid);
              preferences.putString("password", password);
              preferences.end();
              
              preferences.begin("ntp_clock", false);
              preferences.putLong("timezone", timezone);
              preferences.putInt("dst_offset", dstOffset);
              preferences.end();
              
              // Send acknowledgment and flush
              Serial.println("OK:CONFIG_SAVED");
              Serial.flush();
              
              // Small delay to ensure acknowledgment is sent
              delay(100);
              
              configReceived = true;
            }
          }
          configCmd = ""; // Reset for next line
        } else if (c >= 32) {  // Only add printable characters
          configCmd += c;
        }
      }
    }
    delay(50);  // Check every 50ms
  }
  
  // Init Pins
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_CS_DISP, OUTPUT);
  digitalWrite(PIN_CS_DISP, HIGH);
  
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Generate unique AP SSID using MAC address
  WiFi.mode(WIFI_STA);
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  String macSuffix = macAddress.substring(macAddress.length() - 6);
  apSSID = "NTP_Clock_" + macSuffix;
  
  // Load WiFi credentials from Preferences
  preferences.begin("wifi_config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();
  
  if (savedSSID.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
      delay(500);
      wifiAttempts++;
    }
  
    if (WiFi.status() == WL_CONNECTED) {
      beepBlocking(2000, 100);
    }
  }
  
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  // Initialize display
  display.begin();
  delay(200);
  
  preferences.begin("ntp_clock", false);
  displayBrightness = preferences.getInt("brightness", 8);
  use24Hour = preferences.getBool("24hour", true);
  gmtOffset_sec = preferences.getLong("timezone", -28800);
  daylightOffset_sec = preferences.getInt("dst_offset", 0);
  preferences.end();
  
  display.setBrightness(displayBrightness);
  delay(50);
  
  showingVersion = true;
  versionStartTime = millis();
  display.displayText(FIRMWARE_VERSION, true);
  delay(100);
   
   if (WiFi.status() != WL_CONNECTED) {
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
  } else {
    showConnAfterVersion = true;
     
     if (WiFi.status() == WL_CONNECTED) {
       wifiConnected = true;
       beepBlocking(3000, 100);
       
       server.on("/", handleRoot);
       server.on("/config", handleConfig);
       server.on("/save", HTTP_POST, handleSave);
       server.on("/factory-reset", HTTP_POST, handleFactoryReset);
       server.begin();
       
       showIPAddress = true;
       ipDisplayCount = 0;
       lastTimeUpdate = 0;
       
       configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
       delay(1000);
       struct tm timeinfo;
       if (getLocalTime(&timeinfo)) {
         timeSynced = true;
         beepBlocking(2500, 50);
         delay(100);
         beepBlocking(3000, 50);
       }
       
       display.clear();
       delay(500);
     } else {
      display.displayText("Err ");
       beepBlocking(500, 500);
       delay(2000);
       
       apMode = true;
       WiFi.mode(WIFI_AP);
       WiFi.softAP(apSSID.c_str(), AP_PASSWORD);
       delay(500);
       server.on("/", handleRoot);
       server.on("/config", handleConfig);
       server.on("/save", HTTP_POST, handleSave);
       server.on("/factory-reset", HTTP_POST, handleFactoryReset);
       server.begin();
       
      display.displayText("AP  ");
     }
   }
 }
 
void loop() {
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
     server.handleClient();
     handleButtons();
     
    static String ipAddressStr = "";
    static bool ipScrollingStarted = false;
    if (!ipScrollingStarted) {
      ipAddressStr = WiFi.softAPIP().toString();
      display.startScrolling(ipAddressStr.c_str(), 350);
      ipScrollingStarted = true;
    }
   } else {
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
   // Only update password if a new one was provided
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

 