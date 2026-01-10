/*
 * NTP Clock - Firmware v1.0
 * Hardware: ESP32-S3-MINI-1
 * 
 * Features:
 * - WiFi connection to NTP server
 * - Displays time as HHMM on 7-segment display
 * - Decimal point on 100s digit flashes like a colon (seconds indicator)
 * 
 * WiFi Credentials:
 * SSID: salty
 * Password: I4getit2
 */

#include <WiFi.h>
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

// --- WIFI CREDENTIALS ---
const char* ssid = "salty";
const char* password = "I4getit2";

// --- NTP CONFIGURATION ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;  // PST: UTC-8 (8 hours * 3600 seconds)
const int   daylightOffset_sec = 3600;  // PDT: +1 hour for daylight saving time

// --- SIMPLE MAX7219 DRIVER (Display) ---
class SimpleMAX7219 {
private:
  int csPin;
  void writeRegister(uint8_t address, uint8_t value) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); 
    digitalWrite(csPin, LOW);
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();
  }
  
public:
  SimpleMAX7219(int cs) : csPin(cs) {}
  
  void begin() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH); 
    writeRegister(0x0C, 0x01); // Shutdown: normal
    writeRegister(0x0B, 0x03); // Scan limit: 4 digits
    writeRegister(0x09, 0xFF); // Decode mode: Code B
    writeRegister(0x0A, 0x08); // Intensity: medium
    clearDisplay(0);
  }
  
  void clearDisplay(int device) {
    for (int i = 1; i <= 8; i++) writeRegister(i, 0x0F);
  }
  
  void setDigit(int device, int digit, int value, bool dp) {
    if (digit < 0 || digit > 7) return;
    uint8_t code = value & 0x0F;
    if (dp) code |= 0x80;
    writeRegister(digit + 1, code);
  }
  
  void setChar(int device, int digit, char value, bool dp) {
    if (digit < 0 || digit > 7) return;
    uint8_t code = 0x0F; // Blank
    if (value == '-') code = 0x0A;
    else if (value == 'E') code = 0x0B;
    else if (value >= '0' && value <= '9') code = value - '0';
    if (dp) code |= 0x80;
    writeRegister(digit + 1, code);
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

// --- STATE VARIABLES ---
bool wifiConnected = false;
bool timeSynced = false;
unsigned long lastTimeUpdate = 0;
bool colonState = false; // For flashing colon
int displayBrightness = 8; // 0-15, default medium
bool use24Hour = true; // 24-hour format (true) or 12-hour format (false)

// Button state tracking
unsigned long lastButtonPress = 0;
bool lastModeState = false;
bool lastUpState = false;
bool lastDownState = false;

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
  
  // Load saved preferences
  preferences.begin("ntp_clock", false);
  displayBrightness = preferences.getInt("brightness", 8);
  use24Hour = preferences.getBool("24hour", true);
  preferences.end();
  
  // Set initial brightness
  lc.setIntensity(0, displayBrightness);
  
  // Display "8888" test
  lc.setDigit(0, 3, 8, false);
  lc.setDigit(0, 2, 8, false);
  lc.setDigit(0, 1, 8, false);
  lc.setDigit(0, 0, 8, false);
  delay(500);
  lc.clearDisplay(0);
  
  // Startup beep
  tone(PIN_BUZZER, 2000, 100);
  delay(150);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  // Show "Conn" while connecting
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
  } else {
    // WiFi failed - show error
    lc.setChar(0, 0, 'E', false);
    lc.setChar(0, 1, 'r', false);
    lc.setChar(0, 2, 'r', false);
    lc.setChar(0, 3, ' ', false);
    tone(PIN_BUZZER, 500, 500);
  }
  
  delay(500);
  lc.clearDisplay(0);
}

void loop() {
  // Handle button inputs
  handleButtons();
  
  // Update time every second
  if (millis() - lastTimeUpdate >= 1000) {
    lastTimeUpdate = millis();
    
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
        // Flash every second (on for even seconds, off for odd seconds)
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
        lc.setChar(0, 0, 'E', false);
        lc.setChar(0, 1, 'r', false);
        lc.setChar(0, 2, 'r', false);
        lc.setChar(0, 3, ' ', false);
        timeSynced = false;
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
