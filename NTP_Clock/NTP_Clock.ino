/*
 * NTP Clock - Firmware v2.00
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
 * 
 * Configuration:
 * - WiFi credentials configured via AP mode web interface (192.168.4.1)
 * - Timezone configured via web interface or buttons
 * - No default WiFi credentials - must be configured
 */

#define FIRMWARE_VERSION "2.00"

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <SPI.h>
#include <Preferences.h>

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
const char* AP_SSID = "NTP-Clock-AP";
const char* AP_PASSWORD = ""; // Open AP

// --- NTP CONFIGURATION ---
const char* ntpServer = "pool.ntp.org";
long  gmtOffset_sec = 0;  // Will be loaded from Preferences
int   daylightOffset_sec = 3600;  // Default 1 hour for DST

// --- SIMPLE MAX7219 DRIVER (Display) ---
class SimpleMAX7219 {
private:
  int csPin;
  uint8_t decodeMask = 0x00;  // bit0=digit0 ... bit7=digit7

  void writeRegister(uint8_t address, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();
  }

  // MAX7219 no-decode mapping: bit0=A, bit1=B, bit2=C, bit3=D, bit4=E, bit5=F, bit6=G, bit7=DP
  uint8_t raw7seg(char c) {
    switch (c) {
      // digits
      case '0': return 0x3F; // A B C D E F
      case '1': return 0x06; // B C
      case '2': return 0x5B; // A B D E G
      case '3': return 0x4F; // A B C D G
      case '4': return 0x66; // B C F G
      case '5': return 0x6D; // A C D F G
      case '6': return 0x7D; // A C D E F G
      case '7': return 0x07; // A B C
      case '8': return 0x7F; // A B C D E F G
      case '9': return 0x6F; // A B C D F G

      // letters (approx)
      case 'A': case 'a': return 0x77; // A B C E F G
      case 'C': case 'c': return 0x39; // A D E F
      case 'o':           return 0x5C; // C D E G
      case 'n':           return 0x54; // C E G (approx)
      case 'r':           return 0x50; // E G
      case 'E':           return 0x79; // A D E F G
      case 'P':           return 0x73; // A B E F G
      case 'H':           return 0x76; // B C E F G
      case 'L':           return 0x38; // D E F
      case '-':           return 0x40; // G

      case ' ':           return 0x00;
      case '.':           return 0x80; // DP only
      default:            return 0x00;
    }
  }

  bool isCodeBCompatible(char value) {
    return (value >= '0' && value <= '9') ||
           value == '-' || value == 'E' || value == 'H' ||
           value == 'L' || value == 'P' || value == ' ';
  }

  bool decodeEnabledForDigit(int digit) const {
    return (decodeMask & (1 << digit)) != 0;
  }

public:
  SimpleMAX7219(int cs) : csPin(cs) {}

  void setDecodeMode(uint8_t mask) {
    decodeMask = mask;
    writeRegister(0x09, decodeMask);
  }

  void begin() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    delay(50);

    writeRegister(0x0C, 0x01); // normal operation
    delay(10);
    writeRegister(0x0B, 0x03); // scan limit: digits 0..3
    delay(10);

    // Default: decode ON for digits 0..3 (time/version)
    setDecodeMode(0x0F);
    delay(10);

    writeRegister(0x0A, 0x08); // intensity
    delay(10);

    clearDisplay(0);
    delay(20);
  }

  void clearDisplay(int device) {
    for (int i = 1; i <= 8; i++) {
      // If decode is enabled for that digit, blank=0x0F; else blank=0x00
      int digit = i - 1;
      writeRegister(i, decodeEnabledForDigit(digit) ? 0x0F : 0x00);
    }
  }

  void setDigit(int device, int digit, int value, bool dp) {
    if (digit < 0 || digit > 7) return;

    if (decodeEnabledForDigit(digit)) {
      uint8_t code = (uint8_t)(value & 0x0F);
      if (dp) code |= 0x80;
      writeRegister(digit + 1, code);
    } else {
      // raw digit
      char c = (char)('0' + (value % 10));
      uint8_t seg = raw7seg(c);
      if (dp) seg |= 0x80;
      writeRegister(digit + 1, seg);
    }
  }

  void setChar(int device, int digit, char value, bool dp) {
    if (digit < 0 || digit > 7) return;

    if (decodeEnabledForDigit(digit) && isCodeBCompatible(value)) {
      uint8_t code = 0x0F;
      if (value >= '0' && value <= '9') code = value - '0';
      else if (value == '-') code = 0x0A;
      else if (value == 'E') code = 0x0B;
      else if (value == 'H') code = 0x0C;
      else if (value == 'L') code = 0x0D;
      else if (value == 'P') code = 0x0E;
      else if (value == ' ') code = 0x0F;

      if (dp) code |= 0x80;
      writeRegister(digit + 1, code);
    } else {
      uint8_t seg = raw7seg(value);
      if (dp) seg |= 0x80;
      writeRegister(digit + 1, seg);
    }
  }
  
  void displayString(int device, const char* str, int startPos) {
    // Display up to 4 characters starting at startPos
    for (int i = 0; i < 4; i++) {
      int pos = startPos + i;
      if (pos >= 0 && pos < strlen(str)) {
        setChar(device, 3 - i, str[pos], false);
      } else {
        setChar(device, 3 - i, ' ', false);
      }
    }
  }
  
  void setIntensity(int device, int intensity) {
    if (intensity < 0) intensity = 0;
    if (intensity > 15) intensity = 15;
    writeRegister(0x0A, intensity); // Intensity register
  }
};

// --- OBJECTS ---
SimpleMAX7219 lc(PIN_CS_DISP);
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
String getConfigPage();

void setup() {
  // Init Pins
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUZZER,   OUTPUT);
  
  // Set CS High
  pinMode(PIN_CS_DISP, OUTPUT);
  digitalWrite(PIN_CS_DISP, HIGH);

  // Init SPI
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  // Init Display
  lc.begin();
  delay(200); // Give display time to initialize
  
  // Load saved preferences FIRST so brightness is set before version display
  preferences.begin("ntp_clock", false);
  displayBrightness = preferences.getInt("brightness", 8);
  use24Hour = preferences.getBool("24hour", true);
  gmtOffset_sec = preferences.getLong("timezone", 0);
  daylightOffset_sec = preferences.getInt("dst_offset", 3600);
  preferences.end();
  
  // Set initial brightness BEFORE showing version
  lc.setIntensity(0, displayBrightness);
  
  // Force clear display multiple times to remove any initialization artifacts
  lc.clearDisplay(0);
  delay(100);
  lc.clearDisplay(0);
  delay(100);
  
  // Display version at boot (e.g., "2.00") - ALWAYS show regardless of preferences
  // MAX7219 digit order: digit 0 = rightmost, digit 3 = leftmost
  // Display as "2.00" (left to right: 2, dot, 0, 0)
  lc.setDecodeMode(0x0F);  // digits use decode for version
  lc.setDigit(0, 3, 2, false);   // Leftmost: 2
  lc.setDigit(0, 2, 0, true);    // Second: 0 with decimal point
  lc.setDigit(0, 1, 0, false);   // Third: 0
  lc.setDigit(0, 0, 0, false);   // Rightmost: 0
  delay(5000); // Block for 5 seconds to show version
  
  // Startup beep
  tone(PIN_BUZZER, 2000, 100);
  delay(150);
  
  // NOW check for WiFi credentials (AFTER version display is complete)
  preferences.begin("wifi_config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();
  
  if (savedSSID.length() == 0) {
    // No WiFi credentials - start AP mode
    apMode = true;
    lc.setDecodeMode(0x00);   // raw segments on all digits for text
    lc.setChar(0, 0, 'A', false);
    lc.setChar(0, 1, 'P', false);
    lc.setChar(0, 2, ' ', false);
    lc.setChar(0, 3, ' ', false);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    // Start web server
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
    
    tone(PIN_BUZZER, 1500, 200);
  } else {
    // Try to connect to WiFi (AFTER version display is done)
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    // Show "Conn" while connecting
    lc.setDecodeMode(0x00);   // raw segments on all digits for text
    lc.setChar(0, 0, 'C', false);
    lc.setChar(0, 1, 'o', false);
    lc.setChar(0, 2, 'n', false);
    lc.setChar(0, 3, 'n', false);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      tone(PIN_BUZZER, 3000, 100);
      
      // Show IP address twice after connecting
      showIPAddress = true;
      ipDisplayCount = 0;
      lastTimeUpdate = 0; // Reset time update to allow IP display first
      
      // Configure NTP
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      
      // Wait for time sync
      delay(1000);
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        timeSynced = true;
        tone(PIN_BUZZER, 2500, 50);
        delay(100);
        tone(PIN_BUZZER, 3000, 50);
      }
      
      // Clear display before showing IP
      lc.clearDisplay(0);
      delay(500);
    } else {
      // WiFi failed - show error and enter AP mode
      lc.setDecodeMode(0x00);   // raw segments on all digits for text
      lc.setChar(0, 0, 'E', false);
      lc.setChar(0, 1, 'r', false);
      lc.setChar(0, 2, 'r', false);
      lc.setChar(0, 3, ' ', false);
      tone(PIN_BUZZER, 500, 500);
      delay(2000);
      
      apMode = true;
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASSWORD);
      server.on("/", handleRoot);
      server.on("/config", handleConfig);
      server.on("/save", HTTP_POST, handleSave);
      server.begin();
      
      lc.setDecodeMode(0x00);   // raw segments on all digits for text
      lc.setChar(0, 0, 'A', false);
      lc.setChar(0, 1, 'P', false);
      lc.setChar(0, 2, ' ', false);
      lc.setChar(0, 3, ' ', false);
    }
  }
  
  // Don't clear display here - let loop() handle IP display or time display
}

void loop() {
  if (apMode) {
    // Handle web server requests in AP mode
    server.handleClient();
    
    // Still handle buttons for brightness/time format
    handleButtons();
    
    // Scroll IP address across display: "192.168.4.1"
    static unsigned long lastScrollUpdate = 0;
    static int scrollPosition = 0;
    static bool decodeSetForIP = false;
    const char* ipAddress = "192.168.4.1";
    const int scrollDelay = 300; // milliseconds between scroll steps
    
    // Set decode mode to raw for IP scrolling (only once)
    if (!decodeSetForIP) {
      lc.setDecodeMode(0x00);   // raw segments on all digits for text/IP
      decodeSetForIP = true;
    }
    
    if (millis() - lastScrollUpdate >= scrollDelay) {
      lastScrollUpdate = millis();
      
      // Display 4 characters starting at scrollPosition
      for (int i = 0; i < 4; i++) {
        int pos = scrollPosition + i;
        if (pos >= 0 && pos < strlen(ipAddress)) {
          lc.setChar(0, 3 - i, ipAddress[pos], false);
        } else {
          lc.setChar(0, 3 - i, ' ', false);
        }
      }
      
      scrollPosition++;
      // Reset when we've scrolled past the end (add 4 spaces at end for smooth exit)
      if (scrollPosition > strlen(ipAddress) + 3) {
        scrollPosition = -3; // Start 3 positions before start for smooth entry
      }
    }
  } else {
    // Normal operation mode
    handleButtons();
    
    // Show IP address twice after WiFi connects (takes priority over time display)
    if (showIPAddress && wifiConnected) {
      static unsigned long lastIPScrollUpdate = 0;
      static int ipScrollPosition = -3;
      static bool decodeSetForIP = false;
      IPAddress ip = WiFi.localIP();
      char ipStr[16];
      sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      const int ipScrollDelay = 300; // milliseconds between scroll steps
      
      // Set decode mode to raw for IP scrolling (only once)
      if (!decodeSetForIP) {
        lc.setDecodeMode(0x00);   // raw segments on all digits for text/IP
        decodeSetForIP = true;
      }
      
      if (millis() - lastIPScrollUpdate >= ipScrollDelay) {
        lastIPScrollUpdate = millis();
        
        // Display 4 characters starting at ipScrollPosition
        for (int i = 0; i < 4; i++) {
          int pos = ipScrollPosition + i;
          if (pos >= 0 && pos < strlen(ipStr)) {
            lc.setChar(0, 3 - i, ipStr[pos], false);
          } else {
            lc.setChar(0, 3 - i, ' ', false);
          }
        }
        
        ipScrollPosition++;
        // When we've scrolled past the end
        if (ipScrollPosition > strlen(ipStr) + 3) {
          ipScrollPosition = -3; // Reset to start
          ipDisplayCount++;
          if (ipDisplayCount >= 2) {
            // Shown twice, now stop and show time
            showIPAddress = false;
            ipScrollPosition = -3;
            ipDisplayCount = 0;
            decodeSetForIP = false; // Reset flag
            lc.setDecodeMode(0x0F);   // decode digits 0..3 for time
            lc.clearDisplay(0); // Clear before showing time
            delay(500);
          }
        }
      }
      // Don't update time while showing IP
      return;
    }
    
    // Update time every second (only if not showing IP)
    {
      // Update time every second
      if (millis() - lastTimeUpdate >= 1000) {
        lastTimeUpdate = millis();
        
        // Ensure decode mode is set for time display (digits)
        lc.setDecodeMode(0x0F);   // decode digits 0..3
        
        if (wifiConnected && timeSynced) {
          struct tm timeinfo;
          if (getLocalTime(&timeinfo)) {
            // Extract hours and minutes
            int hours = timeinfo.tm_hour;
            int minutes = timeinfo.tm_min;
            int seconds = timeinfo.tm_sec;
            
            // Convert to 12-hour format if needed
            if (!use24Hour) {
              if (hours == 0) hours = 12;      // 12 AM
              else if (hours > 12) hours -= 12; // 1 PM - 11 PM
            }
            
            // Format as HHMM (4 digits)
            int displayValue = (hours * 100) + minutes;
            
            // Extract digits
            int d1 = displayValue % 10;           // Minutes ones (rightmost)
            int d2 = (displayValue / 10) % 10;   // Minutes tens
            int d3 = (displayValue / 100) % 10;  // Hours ones
            int d4 = (displayValue / 1000) % 10; // Hours tens (leftmost)
            
            // Flash colon (decimal point on 100s digit) based on seconds
            bool showColon = (seconds % 2 == 0);
            
            // Display time
            lc.setDigit(0, 3, d1, false);  // Minutes ones (rightmost)
            lc.setDigit(0, 2, d2, false);  // Minutes tens
            lc.setDigit(0, 1, d3, showColon); // Hours ones + colon (decimal point)
            
            // In 12-hour mode, hide leading zero (show blank instead of "0")
            if (!use24Hour && d4 == 0) {
              lc.setChar(0, 0, ' ', false);  // Blank for leading zero in 12-hour mode
            } else {
              lc.setDigit(0, 0, d4, false);  // Hours tens (leftmost)
            }
          } else {
            // Time sync lost - show error
            lc.setDecodeMode(0x00);   // raw segments on all digits for text
            lc.setChar(0, 0, 'E', false);
            lc.setChar(0, 1, 'r', false);
            lc.setChar(0, 2, 'r', false);
            lc.setChar(0, 3, ' ', false);
            timeSynced = false;
          }
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
  
  // MODE button: Toggle 12/24 hour format
  if (btnMode && !lastModeState && (now - lastButtonPress > 200)) {
    use24Hour = !use24Hour;
    preferences.begin("ntp_clock", false);
    preferences.putBool("24hour", use24Hour);
    preferences.end();
    tone(PIN_BUZZER, 2000, 50);
    lastButtonPress = now;
  }
  lastModeState = btnMode;
  
  // UP button: Increase brightness
  if (btnUp && !lastUpState && (now - lastButtonPress > 200)) {
    if (displayBrightness < 15) {
      displayBrightness++;
      lc.setIntensity(0, displayBrightness);
      preferences.begin("ntp_clock", false);
      preferences.putInt("brightness", displayBrightness);
      preferences.end();
      tone(PIN_BUZZER, 1500, 30);
    }
    lastButtonPress = now;
  }
  lastUpState = btnUp;
  
  // DOWN button: Decrease brightness
  if (btnDown && !lastDownState && (now - lastButtonPress > 200)) {
    if (displayBrightness > 0) {
      displayBrightness--;
      lc.setIntensity(0, displayBrightness);
      preferences.begin("ntp_clock", false);
      preferences.putInt("brightness", displayBrightness);
      preferences.end();
      tone(PIN_BUZZER, 1000, 30);
    }
    lastButtonPress = now;
  }
  lastDownState = btnDown;
}

void handleRoot() {
  server.send(200, "text/html", getConfigPage());
}

void handleConfig() {
  server.send(200, "text/html", getConfigPage());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String timezoneStr = server.arg("timezone");
  String dstOffsetStr = server.arg("dst_offset");
  String brightnessStr = server.arg("brightness");
  String hourFormatStr = server.arg("hour_format");
  
  // Save WiFi credentials
  preferences.begin("wifi_config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  
  // Save timezone
  long timezoneOffset = timezoneStr.toInt();
  int dstOffset = dstOffsetStr.toInt();
  preferences.begin("ntp_clock", false);
  preferences.putLong("timezone", timezoneOffset);
  preferences.putInt("dst_offset", dstOffset);
  
  // Save display preferences
  if (brightnessStr.length() > 0) {
    int brightness = brightnessStr.toInt();
    if (brightness >= 0 && brightness <= 15) {
      preferences.putInt("brightness", brightness);
      displayBrightness = brightness;
      lc.setIntensity(0, displayBrightness);
    }
  }
  
  if (hourFormatStr.length() > 0) {
    preferences.putBool("24hour", hourFormatStr == "24");
    use24Hour = (hourFormatStr == "24");
  }
  
  preferences.end();
  
  // Send success page
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Configuration Saved</title>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:#f5f5f5;}";
  html += ".success{background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);text-align:center;}";
  html += "h1{color:#4CAF50;}p{color:#666;margin:20px 0;}</style></head><body>";
  html += "<div class='success'><h1>✓ Configuration Saved</h1>";
  html += "<p>WiFi credentials and preferences have been saved.</p>";
  html += "<p>The device will now reboot and connect to your WiFi network.</p>";
  html += "<p>If connection fails, the device will return to AP mode.</p></div></body></html>";
  
  server.send(200, "text/html", html);
  delay(1000);
  
  // Reboot
  ESP.restart();
}

String getConfigPage() {
  // Load current values
  preferences.begin("wifi_config", false);
  String currentSSID = preferences.getString("ssid", "");
  preferences.end();
  
  preferences.begin("ntp_clock", false);
  long currentTimezone = preferences.getLong("timezone", 0);
  int currentDST = preferences.getInt("dst_offset", 3600);
  int currentBrightness = preferences.getInt("brightness", 8);
  bool current24Hour = preferences.getBool("24hour", true);
  preferences.end();
  
  String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NTP Clock Configuration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 500px;
            margin: 0 auto;
            padding: 30px;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 1.8em;
        }
        .subtitle {
            color: #666;
            margin-bottom: 25px;
            font-size: 0.9em;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-weight: 500;
            font-size: 0.9em;
        }
        input[type="text"],
        input[type="password"],
        select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 1em;
            transition: border-color 0.3s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            margin-top: 10px;
            transition: transform 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
        }
        button:active {
            transform: translateY(0);
        }
        .info {
            background: #f5f5f5;
            border-left: 4px solid #667eea;
            padding: 15px;
            border-radius: 4px;
            margin-top: 20px;
            font-size: 0.9em;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>NTP Clock Configuration</h1>
        <p class="subtitle">Configure WiFi and preferences</p>
        
        <form method="POST" action="/save">
            <div class="form-group">
                <label for="ssid">WiFi SSID *</label>
                <input type="text" id="ssid" name="ssid" value=")" + currentSSID + R"(" placeholder="Enter WiFi network name" required>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password *</label>
                <input type="password" id="password" name="password" placeholder="Enter WiFi password" required>
            </div>
            
            <div class="form-group">
                <label for="timezone">Timezone Offset (seconds from UTC) *</label>
                <select id="timezone" name="timezone" required>
                    <option value="-28800" )" + (currentTimezone == -28800 ? "selected" : "") + R"(>PST (UTC-8)</option>
                    <option value="-25200" )" + (currentTimezone == -25200 ? "selected" : "") + R"(>PDT (UTC-7)</option>
                    <option value="-21600" )" + (currentTimezone == -21600 ? "selected" : "") + R"(>MST (UTC-7)</option>
                    <option value="-18000" )" + (currentTimezone == -18000 ? "selected" : "") + R"(>CST/EST (UTC-6/-5)</option>
                    <option value="-14400" )" + (currentTimezone == -14400 ? "selected" : "") + R"(>EDT/CDT (UTC-4/-5)</option>
                    <option value="0" )" + (currentTimezone == 0 ? "selected" : "") + R"(>UTC</option>
                    <option value="3600" )" + (currentTimezone == 3600 ? "selected" : "") + R"(>CET (UTC+1)</option>
                    <option value="7200" )" + (currentTimezone == 7200 ? "selected" : "") + R"(>CEST (UTC+2)</option>
                    <option value="28800" )" + (currentTimezone == 28800 ? "selected" : "") + R"(>CST China (UTC+8)</option>
                    <option value="32400" )" + (currentTimezone == 32400 ? "selected" : "") + R"(>JST (UTC+9)</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="dst_offset">Daylight Saving Offset (seconds)</label>
                <input type="text" id="dst_offset" name="dst_offset" value=")" + String(currentDST) + R"(" placeholder="3600">
                <small style="color: #666; font-size: 0.85em;">Usually 3600 (1 hour) or 0</small>
            </div>
            
            <div class="form-group">
                <label for="brightness">Display Brightness (0-15)</label>
                <input type="text" id="brightness" name="brightness" value=")" + String(currentBrightness) + R"(" placeholder="8">
            </div>
            
            <div class="form-group">
                <label for="hour_format">Time Format</label>
                <select id="hour_format" name="hour_format">
                    <option value="24" )" + (current24Hour ? "selected" : "") + R"(>24 Hour</option>
                    <option value="12" )" + (!current24Hour ? "selected" : "") + R"(>12 Hour (AM/PM)</option>
                </select>
            </div>
            
            <button type="submit">Save Configuration</button>
        </form>
        
        <div class="info">
            <strong>Note:</strong> After saving, the device will reboot and attempt to connect to your WiFi network. 
            If connection fails, it will return to AP mode.
        </div>
    </div>
</body>
</html>
)";
  return html;
}
