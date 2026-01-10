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
 * 
 * 7-Segment Display Reference for "Conn":
 * 
 * Standard 7-segment layout:
 *     A
 *    F B
 *     G
 *    E C
 *     D
 * 
 * "Conn" spelled out in segments (what SHOULD display):
 * 
 * C:  A, D, E, F  (top, bottom-left, bottom, top-left)
 *     _
 *    |  
 *    |
 *     -
 * 

 * 
 * o:  C, D, E, G  (lowercase o - right side, bottom, bottom-left, middle)
 *     
 *     
 *     -
 *    | |
 *     _
 * 
 * n:  C, E, G  (lowercase n - right side, bottom-left, middle)
 *     
 *     
 *     -
 *    | | 
 * 

 */


 #define FIRMWARE_VERSION "2.14"

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <SPI.h>
#include <Preferences.h>
#include <string.h>
 
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

// MAX7219 Register definitions (for direct register access in IP scrolling)
#define REG_DIGIT0      0x01  // Leftmost
#define REG_DIGIT1      0x02
#define REG_DIGIT2      0x03
#define REG_DIGIT3      0x04  // Rightmost
 
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
  // Match charToSegment() for digits (test_display verified working)
  uint8_t raw7seg(char c) {
    switch (c) {
      // digits - match charToSegment() from test_display
      case '0': return 0x7E; // A B C D E F
      case '1': return 0x30; // B C
      case '2': return 0x6D; // A B D E G
      case '3': return 0x79; // A B C D G
      case '4': return 0x33; // B C F G
      case '5': return 0x5B; // A C D F G
      case '6': return 0x5F; // A C D E F G
      case '7': return 0x70; // A B C
      case '8': return 0x7F; // A B C D E F G
      case '9': return 0x7B; // A B C D F G

      // letters - Segments are REVERSED: bit0->G, bit1->F, bit2->E, bit3->D, bit4->C, bit5->B, bit6->A, bit7->DP
      // To display segment A (top), send bit6. To display B (top-right), send bit5. etc.
      case 'A': case 'a': return 0x77; // A B C E F G -> bit6|bit5|bit4|bit2|bit1|bit0 = 0x40|0x20|0x10|0x04|0x02|0x01 = 0x77
      case 'C': case 'c': return 0x4E; // A D E F -> bit6|bit3|bit2|bit1 = 0x40|0x08|0x04|0x02 = 0x4E
      case 'o':           return 0x1D; // C D E G -> bit4|bit3|bit2|bit0 = 0x10|0x08|0x04|0x01 = 0x1D
      case 'n':           return 0x15; // C E G -> bit4|bit2|bit0 = 0x10|0x04|0x01 = 0x15
      case 'r':           return 0x05; // E G -> bit2|bit0 = 0x04|0x01 = 0x05
      case 'E':           return 0x4F; // A D E F G -> bit6|bit3|bit2|bit1|bit0 = 0x40|0x08|0x04|0x02|0x01 = 0x4F
      case 'P':           return 0x67; // A B E F G -> bit6|bit5|bit2|bit1|bit0 = 0x40|0x20|0x04|0x02|0x01 = 0x67
      case 'H':           return 0x37; // B C E F G -> bit5|bit4|bit2|bit1|bit0 = 0x20|0x10|0x04|0x02|0x01 = 0x37
      case 'L':           return 0x0E; // D E F -> bit3|bit2|bit1 = 0x08|0x04|0x02 = 0x0E
      case '-':           return 0x01; // G -> bit0 = 0x01 (middle segment)

      case ' ':           return 0x00;
      case '.':           return 0x80; // DP only
      default:            return 0x00;
    }
  }
  
  // Helper function for IP scrolling: convert char to segment pattern (for digits in IP)
  uint8_t charToSegment(char c) {
    switch (c) {
      case '0': return 0x7E;
      case '1': return 0x30;
      case '2': return 0x6D;
      case '3': return 0x79;
      case '4': return 0x33;
      case '5': return 0x5B;
      case '6': return 0x5F;
      case '7': return 0x70;
      case '8': return 0x7F;
      case '9': return 0x7B;
      case '-': return 0x01;
      case ' ': return 0x00;
      default:  return 0x00;
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
 
   // Bit-bang a single register write (for early init before SPI is reliable)
   void bitBangWrite(uint8_t address, uint8_t value) {
     // Use GPIO directly - much slower but more reliable at boot
     digitalWrite(csPin, LOW);
     delayMicroseconds(10);
     
     // Send address byte (MSB first)
     for (int i = 7; i >= 0; i--) {
       digitalWrite(PIN_SPI_SCK, LOW);
       delayMicroseconds(5);
       digitalWrite(PIN_SPI_MOSI, (address >> i) & 0x01);
       delayMicroseconds(5);
       digitalWrite(PIN_SPI_SCK, HIGH);
       delayMicroseconds(5);
     }
     
     // Send value byte (MSB first)
     for (int i = 7; i >= 0; i--) {
       digitalWrite(PIN_SPI_SCK, LOW);
       delayMicroseconds(5);
       digitalWrite(PIN_SPI_MOSI, (value >> i) & 0x01);
       delayMicroseconds(5);
       digitalWrite(PIN_SPI_SCK, HIGH);
       delayMicroseconds(5);
     }
     
     digitalWrite(PIN_SPI_SCK, LOW);
     delayMicroseconds(10);
     digitalWrite(csPin, HIGH);
     delayMicroseconds(50);
   }
 
  void begin() {
    // Match test_display's initDisplay() exactly - use hardware SPI only
    // CS pin is already configured HIGH before this is called
    
    writeRegister(0x0F, 0x00);  // Test mode OFF
    delay(10);
    writeRegister(0x0C, 0x00);  // Shutdown ON
    delay(10);
    writeRegister(0x0B, 0x03);  // Scan limit: digits 0..3
    delay(10);
    writeRegister(0x09, 0x00);  // Decode mode: raw (will be set later as needed)
    decodeMask = 0x00;
    delay(10);
    writeRegister(0x0A, 0x08);  // Intensity
    delay(10);
    
    // Clear all digit registers
    for (int i = 1; i <= 4; i++) {
      writeRegister(i, 0x00);
      delay(5);
    }
    
    // Wake up - shutdown mode OFF
    writeRegister(0x0C, 0x01);
    delay(50);
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
  
  // Public method to write raw segment pattern directly (for IP scrolling)
  void writeRawSegment(int device, int digit, uint8_t segments) {
    if (digit < 0 || digit > 7) return;
    writeRegister(digit + 1, segments);
  }
  
  // Public method to get charToSegment conversion (for IP scrolling)
  uint8_t getCharSegment(char c) {
    return charToSegment(c);
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
bool showingVersion = true; // Guard flag to prevent loop() from overwriting version display
unsigned long versionStartTime = 0; // When version display started
 
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
 String getConfigPage();
 
void setup() {
  // === MATCH test_display EXACTLY ===
  Serial.begin(115200);
  delay(2000);
  
  // Init Pins - MATCH test_display order
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_CS_DISP, OUTPUT);  // CS pin BEFORE WiFi (like test_display)
  digitalWrite(PIN_CS_DISP, HIGH);
  
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  digitalWrite(PIN_BUZZER, LOW);
  
  // === WIFI INIT ===
  // Match test_display: Initialize WiFi and WAIT for connection BEFORE display init
  // Load WiFi credentials from Preferences (saved in "wifi_config" namespace)
  preferences.begin("wifi_config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();
  
  Serial.println("Loaded SSID: " + savedSSID);
  Serial.println("Password length: " + String(savedPassword.length()));
  
  if (savedSSID.length() > 0) {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  
  // Wait for WiFi connection (up to 15 seconds) - MATCHES test_display
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
      tone(PIN_BUZZER, 2000, 100);
    } else {
      Serial.println("\nWiFi failed!");
    }
  } else {
    Serial.println("No WiFi credentials saved - will start in AP mode");
  }
  
  // === NOW init SPI and display (AFTER WiFi) ===
  // Match test_display exactly
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  // Init Display - use the class method
  lc.begin();
  delay(200);
   
   
   // Clear and prepare for version display
   delay(100);
   
   // Load saved preferences FIRST so brightness is set before version display
   preferences.begin("ntp_clock", false);
   displayBrightness = preferences.getInt("brightness", 8);
   use24Hour = preferences.getBool("24hour", true);
   // Default to PST (UTC-8) if not set - Pacific Standard Time
   gmtOffset_sec = preferences.getLong("timezone", -28800);  // PST = UTC-8 = -28800 seconds
   // Default DST offset is 0 (no DST applied by default - user must configure)
   daylightOffset_sec = preferences.getInt("dst_offset", 0);
   preferences.end();
   
   // Set initial brightness
   lc.setIntensity(0, displayBrightness);
   delay(50);
   
  // Display version at boot - ALWAYS show regardless of preferences
  // Parse FIRMWARE_VERSION (e.g., "2.13") and display right-justified
  showingVersion = true;
  versionStartTime = millis();
  
  lc.setDecodeMode(0x0F);  // digits use decode for version
  delay(10);
  
  // Parse version string (format: "X.XX")
  String version = String(FIRMWARE_VERSION);
  int dotPos = version.indexOf('.');
  int major = version.substring(0, dotPos).toInt();
  int minor1 = version.substring(dotPos + 1, dotPos + 2).toInt();
  int minor2 = version.substring(dotPos + 2).toInt();
  
  // Right-justified: blank, major, dot+minor1, minor2
  lc.setDigit(0, 0, 0x0F, false);      // Leftmost: blank (DIGIT0)
  lc.setDigit(0, 1, major, true);      // Second: major with decimal point (DIGIT1)
  lc.setDigit(0, 2, minor1, false);    // Third: minor1 (DIGIT2)
  lc.setDigit(0, 3, minor2, false);    // Rightmost: minor2 (DIGIT3)
  
  // Block for 5 seconds to show version (loop() will be guarded by showingVersion flag)
  delay(5000);
  
  // Clear version display flag and clear display
  showingVersion = false;
  lc.clearDisplay(0);
  delay(100);
   
   // Startup beep
   tone(PIN_BUZZER, 2000, 100);
   delay(150);
   
   // WiFi connection was attempted above - check status
   // If connection failed, fall back to AP mode
   if (WiFi.status() != WL_CONNECTED) {
     // Connection failed - fall back to AP mode
    apMode = true;
    lc.setDecodeMode(0x00);   // raw segments on all digits for text
    lc.setChar(0, 0, 'A', false);  // Leftmost: A (DIGIT0)
    lc.setChar(0, 1, 'P', false);  // Second: P (DIGIT1)
    lc.setChar(0, 2, ' ', false);  // Third: blank (DIGIT2)
    lc.setChar(0, 3, ' ', false);  // Rightmost: blank (DIGIT3)
     
     // Explicitly set WiFi mode to AP before starting
     WiFi.mode(WIFI_AP);
     WiFi.softAP(AP_SSID, AP_PASSWORD);
     delay(500); // Give AP time to start
     Serial.println("AP started: " + WiFi.softAPIP().toString());
     Serial.println("AP SSID: " + String(AP_SSID));
     
     // Start web server
     server.on("/", handleRoot);
     server.on("/config", handleConfig);
     server.on("/save", HTTP_POST, handleSave);
     server.on("/factory-reset", HTTP_POST, handleFactoryReset);
     server.begin();
     Serial.println("Web server started");
     
     tone(PIN_BUZZER, 1500, 200);
  } else {
    // WiFi connection succeeded (already connected from setup)
    // Show "Conn" briefly, then proceed
    // DIGIT0 = leftmost, DIGIT3 = rightmost
    lc.setDecodeMode(0x00);   // raw segments on all digits for text
    lc.setChar(0, 0, 'C', false);  // Leftmost: C (DIGIT0)
    lc.setChar(0, 1, 'o', false); // Second: o (DIGIT1)
    lc.setChar(0, 2, 'n', false);  // Third: n (DIGIT2)
    lc.setChar(0, 3, 'n', false);  // Rightmost: n (DIGIT3)
    delay(1000);  // Show "Conn" briefly
     
     if (WiFi.status() == WL_CONNECTED) {
       wifiConnected = true;
       tone(PIN_BUZZER, 3000, 100);
       
       // Start web server for configuration access
       server.on("/", handleRoot);
       server.on("/config", handleConfig);
       server.on("/save", HTTP_POST, handleSave);
       server.on("/factory-reset", HTTP_POST, handleFactoryReset);
       server.begin();
       Serial.println("Web server started on IP: " + WiFi.localIP().toString());
       
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
      lc.setChar(0, 0, 'E', false);  // Leftmost: E (DIGIT0)
      lc.setChar(0, 1, 'r', false);  // Second: r (DIGIT1)
      lc.setChar(0, 2, 'r', false);  // Third: r (DIGIT2)
      lc.setChar(0, 3, ' ', false);  // Rightmost: blank (DIGIT3)
       tone(PIN_BUZZER, 500, 500);
       delay(2000);
       
       apMode = true;
       WiFi.mode(WIFI_AP);
       WiFi.softAP(AP_SSID, AP_PASSWORD);
       delay(500); // Give AP time to start
       Serial.println("AP started: " + WiFi.softAPIP().toString());
       Serial.println("AP SSID: " + String(AP_SSID));
       server.on("/", handleRoot);
       server.on("/config", handleConfig);
       server.on("/save", HTTP_POST, handleSave);
       server.on("/factory-reset", HTTP_POST, handleFactoryReset);
       server.begin();
       Serial.println("Web server started");
       
      lc.setDecodeMode(0x00);   // raw segments on all digits for text
      lc.setChar(0, 0, 'A', false);  // Leftmost: A (DIGIT0)
      lc.setChar(0, 1, 'P', false);  // Second: P (DIGIT1)
      lc.setChar(0, 2, ' ', false);  // Third: blank (DIGIT2)
      lc.setChar(0, 3, ' ', false);  // Rightmost: blank (DIGIT3)
     }
   }
   
   // Don't clear display here - let loop() handle IP display or time display
 }
 
void loop() {
  // Guard: Don't do anything if we're still showing version
  if (showingVersion) {
    if (millis() - versionStartTime >= 5000) {
      showingVersion = false;
      lc.clearDisplay(0);
    }
    return; // Exit early - nothing else should touch the display
  }
  
  if (apMode) {
     // Handle web server requests in AP mode
     server.handleClient();
     
     // Still handle buttons for brightness/time format
     handleButtons();
     
    // Scroll IP address across display - get actual AP IP
    // Process IP string to merge dots with previous digits (like test_display)
    static unsigned long lastScrollUpdate = 0;
    static int scrollPosition = 0;
    static char displayStr[24];  // Processed string (dots merged with prev digit)
    static uint8_t dpMask[24];   // Which positions have decimal points
    static int displayLen = 0;
    static bool initialized = false;
    static String ipAddressStr = "";
    if (ipAddressStr.length() == 0) {
      ipAddressStr = WiFi.softAPIP().toString();
    }
    const char* ipAddress = ipAddressStr.c_str();
    const int scrollDelay = 350; // milliseconds between scroll steps
    
    // Set decode mode to raw for IP scrolling (only once)
    static bool decodeSetForIP = false;
    if (!decodeSetForIP) {
      lc.setDecodeMode(0x00);   // raw segments on all digits for text/IP
      decodeSetForIP = true;
    }
    
    // Build display string once (remove dots, mark DP positions)
    if (!initialized) {
      displayLen = 0;
      memset(dpMask, 0, sizeof(dpMask));
      
      // Add leading spaces
      for (int i = 0; i < 4; i++) {
        displayStr[displayLen++] = ' ';
      }
      
      // Copy IP, merging dots
      for (int i = 0; ipAddress[i] != '\0'; i++) {
        if (ipAddress[i] == '.') {
          if (displayLen > 0) {
            dpMask[displayLen - 1] = 1;  // Add DP to previous char
          }
        } else {
          displayStr[displayLen++] = ipAddress[i];
        }
      }
      
      // Add trailing spaces
      for (int i = 0; i < 4; i++) {
        displayStr[displayLen++] = ' ';
      }
      displayStr[displayLen] = '\0';
      initialized = true;
    }
    
    if (millis() - lastScrollUpdate >= scrollDelay) {
      lastScrollUpdate = millis();
      
      // Display 4 characters starting at scrollPosition
      // DIGIT0 = leftmost, DIGIT3 = rightmost
      for (int i = 0; i < 4; i++) {
        int strPos = scrollPosition + i;
        uint8_t seg = 0x00;
        
        if (strPos >= 0 && strPos < displayLen) {
          seg = lc.getCharSegment(displayStr[strPos]);
          if (dpMask[strPos]) {
            seg |= 0x80;  // Add decimal point
          }
        }
        
        // DIGIT0 = leftmost, so first char goes to DIGIT0
        lc.writeRawSegment(0, i, seg);
      }
      
      scrollPosition++;
      if (scrollPosition > displayLen - 4) {
        scrollPosition = 0;  // Reset scroll position
      }
    }
   } else {
     // Normal operation mode (connected to WiFi)
     // Handle web server requests even when connected to WiFi
     server.handleClient();
     
     handleButtons();
     
    // Show IP address twice after WiFi connects (takes priority over time display)
    if (showIPAddress && wifiConnected) {
      static unsigned long lastIPScrollUpdate = 0;
      static int ipScrollPosition = 0;
      static char ipDisplayStr[24];  // Processed string (dots merged with prev digit)
      static uint8_t ipDpMask[24];   // Which positions have decimal points
      static int ipDisplayLen = 0;
      static bool ipInitialized = false;
      static bool decodeSetForIP = false;
      IPAddress ip = WiFi.localIP();
      char ipStr[16];
      sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      const int ipScrollDelay = 350; // milliseconds between scroll steps
      
      // Set decode mode to raw for IP scrolling (only once)
      if (!decodeSetForIP) {
        lc.setDecodeMode(0x00);   // raw segments on all digits for text/IP
        decodeSetForIP = true;
      }
      
      // Build display string once (remove dots, mark DP positions)
      if (!ipInitialized) {
        ipDisplayLen = 0;
        memset(ipDpMask, 0, sizeof(ipDpMask));
        
        // Add leading spaces
        for (int i = 0; i < 4; i++) {
          ipDisplayStr[ipDisplayLen++] = ' ';
        }
        
        // Copy IP, merging dots
        for (int i = 0; ipStr[i] != '\0'; i++) {
          if (ipStr[i] == '.') {
            if (ipDisplayLen > 0) {
              ipDpMask[ipDisplayLen - 1] = 1;  // Add DP to previous char
            }
          } else {
            ipDisplayStr[ipDisplayLen++] = ipStr[i];
          }
        }
        
        // Add trailing spaces
        for (int i = 0; i < 4; i++) {
          ipDisplayStr[ipDisplayLen++] = ' ';
        }
        ipDisplayStr[ipDisplayLen] = '\0';
        ipInitialized = true;
      }
      
      if (millis() - lastIPScrollUpdate >= ipScrollDelay) {
        lastIPScrollUpdate = millis();
        
        // Display 4 characters starting at ipScrollPosition
        // DIGIT0 = leftmost, DIGIT3 = rightmost
        for (int i = 0; i < 4; i++) {
          int strPos = ipScrollPosition + i;
          uint8_t seg = 0x00;
          
          if (strPos >= 0 && strPos < ipDisplayLen) {
            seg = lc.getCharSegment(ipDisplayStr[strPos]);
            if (ipDpMask[strPos]) {
              seg |= 0x80;  // Add decimal point
            }
          }
          
          // DIGIT0 = leftmost, so first char goes to DIGIT0
          lc.writeRawSegment(0, i, seg);
        }
        
        ipScrollPosition++;
        // When we've scrolled past the end
        if (ipScrollPosition > ipDisplayLen - 4) {
          ipScrollPosition = 0; // Reset to start
          ipDisplayCount++;
          if (ipDisplayCount >= 2) {
            // Shown twice, now stop and show time
            showIPAddress = false;
            ipScrollPosition = 0;
            ipDisplayCount = 0;
            ipInitialized = false; // Reset for next time
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
            
            // Display time - DIGIT0 = leftmost, DIGIT3 = rightmost
            // In 12-hour mode, hide leading zero (show blank instead of "0")
            // DIGIT0 = leftmost, DIGIT3 = rightmost
            if (!use24Hour && d4 == 0) {
              lc.setChar(0, 0, ' ', false);  // Blank for leading zero in 12-hour mode (DIGIT0 = leftmost)
            } else {
              lc.setDigit(0, 0, d4, false);  // Hours tens (leftmost, DIGIT0)
            }
            lc.setDigit(0, 1, d3, showColon); // Hours ones + colon (DIGIT1)
            lc.setDigit(0, 2, d2, false);  // Minutes tens (DIGIT2)
            lc.setDigit(0, 3, d1, false);  // Minutes ones (rightmost, DIGIT3)
           } else {
            // Time sync lost - show error
            lc.setDecodeMode(0x00);   // raw segments on all digits for text
            lc.setChar(0, 0, 'E', false);  // Leftmost: E (DIGIT0)
            lc.setChar(0, 1, 'r', false);  // Second: r (DIGIT1)
            lc.setChar(0, 2, 'r', false);  // Third: r (DIGIT2)
            lc.setChar(0, 3, ' ', false);  // Rightmost: blank (DIGIT3)
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
   
   Serial.println("=== Saving Configuration ===");
   Serial.println("SSID: " + ssid);
   Serial.println("Password length: " + String(password.length()));
   Serial.println("Timezone: " + timezoneStr);
   
   // Save WiFi credentials
   preferences.begin("wifi_config", false);
   preferences.putString("ssid", ssid);
   preferences.putString("password", password);
   preferences.end();
   
   Serial.println("WiFi credentials saved to 'wifi_config' namespace");
   
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
 
 void handleFactoryReset() {
   Serial.println("=== Factory Reset Requested ===");
   
   // Clear all preferences from wifi_config namespace
   preferences.begin("wifi_config", false);
   preferences.clear();
   preferences.end();
   Serial.println("Cleared wifi_config preferences");
   
   // Clear all preferences from ntp_clock namespace
   preferences.begin("ntp_clock", false);
   preferences.clear();
   preferences.end();
   Serial.println("Cleared ntp_clock preferences");
   
   // Send confirmation page
   String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Factory Reset Complete</title>";
   html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:50px auto;padding:20px;background:#f5f5f5;}";
   html += ".success{background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);text-align:center;}";
   html += "h1{color:#dc3545;}p{color:#666;margin:20px 0;}</style></head><body>";
   html += "<div class='success'><h1>✓ Factory Reset Complete</h1>";
   html += "<p>All preferences have been cleared.</p>";
   html += "<p>The device will now reboot and return to AP mode.</p></div></body></html>";
   
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
        
        <form method="POST" action="/factory-reset" style="margin-top: 20px;">
            <button type="submit" style="background-color: #dc3545; color: white; width: 100%; padding: 12px; border: none; border-radius: 8px; font-size: 1em; cursor: pointer; font-weight: 500;">Factory Reset</button>
        </form>
    </div>
</body>
</html>
 )";
   return html;
 }
 