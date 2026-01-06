/*
 * Column Temperature Monitor - Firmware v3.0 (As-Built)
 * Hardware: ESP32-S3-MINI-1 (Custom PCBA)
 * 
 * Pin Mapping (from Netlist_Schematic1_2025-12-28-final-enet.enet):
 * Physical Pin → GPIO → Function
 * Pin 4  → GPIO 0  → Boot Button (SW2)
 * Pin 8  → GPIO 4  → Buzzer Control (Q1 Gate)
 * Pin 9  → GPIO 7  → Button Down (SW5)
 * Pin 10 → GPIO 6  → Button Up (SW4)
 * Pin 11 → GPIO 5  → Button Mode (SW3)
 * Pin 14 → GPIO 10 → CS_RTD (MAX31865 Chip Select)
 * Pin 15 → GPIO 11 → CS_DISP (MAX7219 Chip Select)
 * Pin 20 → GPIO 16 → SPI SCK (Clock)
 * Pin 21 → GPIO 17 → SPI MOSI (Master Out)
 * Pin 22 → GPIO 18 → SPI MISO (Master In)
 * Pin 23 → GPIO 19 → USB D-
 * Pin 24 → GPIO 20 → USB D+
 * Pin 39 → GPIO 43 → UART TX
 * Pin 40 → GPIO 44 → UART RX
 */

#include <SPI.h>
#include <Adafruit_MAX31865.h> 
#include <Preferences.h>       

// --- PIN DEFINITIONS (From Netlist Physical Pin → GPIO Mapping) ---

// User Inputs (Physical Pins 8-11 map to GPIO 4-7)
// Physical wiring: GPIO 5=low tone, GPIO 6=medium tone, GPIO 7=high tone
// Remapped so: MODE=high, UP=medium, DOWN=low
const int PIN_BTN_MODE = 7;  // GPIO 7 → Physical button that plays high tone → call it MODE
const int PIN_BTN_UP   = 6;  // GPIO 6 → Physical button that plays medium tone → call it UP
const int PIN_BTN_DOWN = 5;  // GPIO 5 → Physical button that plays low tone → call it DOWN
const int PIN_BUZZER   = 4;  // GPIO 4 (Physical Pin 8)  → Q1 Gate

// SPI Bus (Physical Pins 20-22 map to GPIO 16-18)
const int PIN_SPI_SCK  = 16;  // GPIO 16 (Physical Pin 20) → SPI SCK
const int PIN_SPI_MOSI = 17;  // GPIO 17 (Physical Pin 21) → SPI MOSI
const int PIN_SPI_MISO = 18;  // GPIO 18 (Physical Pin 22) → SPI MISO

// Chip Selects (Physical Pins 14-15 map to GPIO 10-11)
const int PIN_CS_RTD   = 10;  // GPIO 10 (Physical Pin 14) → CS_RTD (MAX31865)
const int PIN_CS_DISP  = 11;  // GPIO 11 (Physical Pin 15) → CS_DISP (MAX7219)

// --- CONSTANTS ---
const float R_REF     = 430.0; // Reference Resistor (R4)
const float R_NOMINAL = 100.0; // PT100
const float TEMP_MAX  = 999.0; 

// --- SIMPLE MAX7219 DRIVER CLASS ---
class SimpleMAX7219 {
private:
  int csPin;
  void writeRegister(uint8_t address, uint8_t value) {
    // Use SPI transactions to prevent conflicts with other SPI devices
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); // 10MHz, MSB first, Mode 0
    digitalWrite(csPin, LOW);
    delayMicroseconds(1);
    SPI.transfer(address);
    SPI.transfer(value);
    delayMicroseconds(1);
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();
    delayMicroseconds(10); // Small delay to ensure CS is stable
  }
  
public:
  SimpleMAX7219(int cs) : csPin(cs) {}
  
  void begin() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);
    writeRegister(0x0C, 0x01); // Shutdown: normal operation
    writeRegister(0x0B, 0x03);  // Scan limit: 4 digits
    writeRegister(0x09, 0xFF);  // Decode mode: Code B for all digits
    writeRegister(0x0A, 0x08);  // Intensity: medium
    clearDisplay(0);
  }
  
  void shutdown(int device, bool state) {
    writeRegister(0x0C, state ? 0x00 : 0x01);
  }
  
  void setIntensity(int device, int intensity) {
    if (intensity < 0) intensity = 0;
    if (intensity > 15) intensity = 15;
    writeRegister(0x0A, intensity);
  }
  
  void clearDisplay(int device) {
    for (int i = 1; i <= 8; i++) {
      writeRegister(i, 0x0F); // Blank (0x0F in Code B = blank)
    }
  }
  
  void setDigit(int device, int digit, int value, bool dp) {
    if (digit < 0 || digit > 7) return;
    uint8_t code = value & 0x0F;
    if (dp) {
      code |= 0x80; // Set decimal point bit
    }
    writeRegister(digit + 1, code);
  }
  
  void setChar(int device, int digit, char value, bool dp) {
    if (digit < 0 || digit > 7) return;
    uint8_t code = 0x0F; // Blank by default
    
    // Code B decode table for 7-segment
    if (value == '-') code = 0x0A;
    else if (value == ' ') code = 0x0F; // Blank
    else if (value >= '0' && value <= '9') code = value - '0';
    else if (value >= 'A' && value <= 'F') code = value - 'A' + 10;
    else if (value >= 'a' && value <= 'f') code = value - 'a' + 10;
    
    if (dp) code |= 0x80;
    writeRegister(digit + 1, code);
  }
};

// --- OBJECTS ---
// Using Hardware SPI - ESP32-S3 can use custom pins with Hardware SPI
// Initialize SPI.begin() with custom pins, then use Hardware SPI mode (CS only)
Adafruit_MAX31865 thermo = Adafruit_MAX31865(PIN_CS_RTD); // Hardware SPI mode
SimpleMAX7219 lc(PIN_CS_DISP);
Preferences preferences;

// --- STATE VARIABLES ---
enum SystemMode { MODE_RUN, MODE_SET_THRESH, MODE_SET_STEP };
SystemMode currentMode = MODE_RUN;

float configThreshold = 170.0; 
float configStepSize  = 0.5;   
int currentTempInt    = 0;
int   lastBandIndex   = -1;

// Input Debounce
unsigned long lastInputTime = 0;
unsigned long buttonHoldStart = 0;
bool buttonHeld = false;
const int BTN_DELAY_INITIAL = 400; 
const int BTN_DELAY_REPEAT  = 100; 

void setup() {
  // Init Pins
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUZZER,   OUTPUT);

  // Init Hardware SPI with custom pins
  // This configures the default SPI bus (VSPI) to use our custom pins
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  // Init Display (uses same Hardware SPI bus)
  lc.begin();
  
  // Test display with "8888" to verify all 4 digits work
  lc.setDigit(0, 3, 8, false);
  lc.setDigit(0, 2, 8, false);
  lc.setDigit(0, 1, 8, false);
  lc.setDigit(0, 0, 8, false);
  delay(1000);
  
  // Test each digit individually: 0123
  lc.setDigit(0, 3, 3, false);  // Rightmost = 3
  lc.setDigit(0, 2, 2, false);  // 
  lc.setDigit(0, 1, 1, false);  // 
  lc.setDigit(0, 0, 0, false);  // Leftmost = 0
  delay(1000);
  
  // Test reverse: 3210
  lc.setDigit(0, 3, 0, false);  // Rightmost = 0
  lc.setDigit(0, 2, 1, false);  // 
  lc.setDigit(0, 1, 2, false);  // 
  lc.setDigit(0, 0, 3, false);  // Leftmost = 3
  delay(1000);
  
  lc.clearDisplay(0);

  // Startup Chirp (Confirm Life)
  tone(PIN_BUZZER, 2000, 100);
  delay(150);
  tone(PIN_BUZZER, 3000, 100);
  delay(500);
  
  // TEST 1: Try to initialize RTD sensor with timeout protection
  // The Adafruit_MAX31865 uses Software SPI, so it shouldn't conflict
  // But maybe begin() is hanging - let's see if we can detect that
  tone(PIN_BUZZER, 1500, 50); // Mid beep = about to try RTD init
  delay(100);
  
  // Try RTD init - if this hangs, we won't hear the result beep
  bool rtdOk = false;
  // Wrap in try-catch? No, Arduino doesn't have exceptions
  // Just call it and see if we get past it
  rtdOk = thermo.begin(MAX31865_3WIRE);
  
  if (rtdOk) {
    tone(PIN_BUZZER, 2500, 50); // High beep = RTD init OK
  } else {
    tone(PIN_BUZZER, 800, 50); // Low beep = RTD init failed
  }
  delay(100);
}

// Function to read MAX31865 register directly via SPI (for debugging)
uint8_t readMAX31865Register(uint8_t reg) {
  // Use SPI transactions to prevent conflicts with display
  SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE1)); // 5MHz, MSB first, Mode 1 for MAX31865
  digitalWrite(PIN_CS_RTD, LOW);
  delayMicroseconds(1);
  SPI.transfer(reg & 0x7F); // Read command: bit 7 = 0 for read
  uint8_t value = SPI.transfer(0x00); // Dummy byte to read data
  delayMicroseconds(1);
  digitalWrite(PIN_CS_RTD, HIGH);
  SPI.endTransaction();
  delayMicroseconds(10); // Small delay to ensure CS is stable
  return value;
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastTempRead = 0;
  static int currentTempInt = 0;
  static bool lastModeState = false;
  static bool lastUpState = false;
  static bool lastDownState = false;
  static bool rtdInitialized = false;
  static unsigned long lastConfigRead = 0;
  static bool beepScheduled = false; // Global flag for scheduled beeps
  static unsigned long lastFaultBeep = 0;
  static unsigned long lastBeepTime = 0;
  
  // Check buttons (active LOW due to pull-up)
  bool btnMode = (digitalRead(PIN_BTN_MODE) == LOW);
  bool btnUp   = (digitalRead(PIN_BTN_UP) == LOW);
  bool btnDown = (digitalRead(PIN_BTN_DOWN) == LOW);
  
  // Detect button presses (edge detection)
  // GPIO assignments swapped: GPIO 7=MODE (high), GPIO 6=UP (medium), GPIO 5=DOWN (low)
  if (btnMode && !lastModeState) {
    // MODE button (GPIO 7) - high tone
    tone(PIN_BUZZER, 2500, 150);
  }
  if (btnUp && !lastUpState) {
    // UP button (GPIO 6) - medium tone
    tone(PIN_BUZZER, 1500, 150);
  }
  if (btnDown && !lastDownState) {
    // DOWN button (GPIO 5) - low tone
    tone(PIN_BUZZER, 800, 150);
  }
  
  // Save button states for next iteration
  lastModeState = btnMode;
  lastUpState = btnUp;
  lastDownState = btnDown;
  
  // Initialize RTD if not done yet
  if (!rtdInitialized) {
    rtdInitialized = thermo.begin(MAX31865_3WIRE);
    if (!rtdInitialized) {
      // Show error on display
      displayInt(9999);
      delay(100);
      return;
    }
    // After initialization, immediately read and display config register
    delay(100);
    uint8_t configReg = readMAX31865Register(0x00);
    // Display config register as CXXX (e.g., C0A3 = config 0xA3)
    // For 3-wire mode, expect bit 4 = 1 (0x10), bit 0 = 1 (bias on)
    // Typical value: 0xA3 or 0xB3
    currentTempInt = 0xC000 + configReg;
    displayInt(currentTempInt);
    delay(3000); // Show config register for 3 seconds after init
  }
  
  // Display config register every 5 seconds to verify chip configuration
  static unsigned long lastConfigCheck = 0;
  static bool showingConfig = false;
  static unsigned long configDisplayStart = 0;
  
  if (millis() - lastConfigCheck >= 5000) {
    lastConfigCheck = millis();
    uint8_t configReg = readMAX31865Register(0x00);
    // Display as CXXX where XXX is config register value
    // For 3-wire mode: bit 4 should be 1 (0x10), bit 0 should be 1 (bias on)
    // Expected: 0xA3 or 0xB3 (depends on other settings)
    currentTempInt = 0xC000 + configReg;
    showingConfig = true;
    configDisplayStart = millis();
    displayInt(currentTempInt);
  }
  
  // Show config for 1 second, then return to RTD reading
  if (showingConfig && (millis() - configDisplayStart > 1000)) {
    showingConfig = false;
  }
  
  if (showingConfig) {
    displayInt(currentTempInt);
    return; // Don't read RTD while showing config
  }
  /*
  // Read configuration register every 5 seconds to verify SPI communication
  static bool showingConfig = false;
  static unsigned long configDisplayStart = 0;
  
  if (millis() - lastConfigRead >= 5000) {
    lastConfigRead = millis();
    uint8_t configReg = readMAX31865Register(0x00);
    currentTempInt = 0xC000 + configReg;
    showingConfig = true;
    configDisplayStart = millis();
    displayInt(currentTempInt);
  }
  
  if (showingConfig && (millis() - configDisplayStart > 2000)) {
    showingConfig = false;
  }
  
  if (showingConfig) {
    displayInt(currentTempInt);
    return;
  }
  */
  if (millis() - lastTempRead >= 500) {
    lastTempRead = millis();
    
    // Check for faults FIRST, before reading RTD
    uint8_t fault = thermo.readFault();
    
    if (fault) {
      // Fault detected - display fault code bits
      // Fault register bits: 
      // Bit 7 = RTD High Threshold (RTD resistance too high)
      // Bit 6 = RTD Low Threshold (RTD resistance too low - often means open circuit)
      // Bit 5 = REFIN- > 0.85*Vbias
      // Bit 4 = REFIN- < 0.85*Vbias  
      // Bit 3 = RTDIN- < 0.85*Vbias (often means open circuit)
      // Bit 2 = Overvoltage/Undervoltage
      // Fault codes are 8-bit (0-255), mask to ensure valid range
      uint8_t faultCode = fault & 0xFF;
      // Display as 90XX where XX is fault code (decimal, 0-255)
      currentTempInt = 9000 + faultCode; 
      // Clear fault after reading so we can see if it persists
      thermo.clearFault();
      // Beep pattern: quiet, slow beeps = fault detected
      // Schedule beep for later (outside SPI operations) to avoid conflicts
      if (millis() - lastFaultBeep > 5000) { // Much slower: every 5 seconds
        beepScheduled = true;
        lastFaultBeep = millis();
      }
    } else {
      // No fault - read RTD value
      // Read multiple times and filter out obviously bad readings
      uint16_t rtd1 = thermo.readRTD();
      delayMicroseconds(100);
      uint16_t rtd2 = thermo.readRTD();
      delayMicroseconds(100);
      uint16_t rtd3 = thermo.readRTD();
      
      // Filter: reject readings that are too low (< 1000 = < 13Ω) or too high (> 20000 = > 260Ω)
      // These are likely noise or bad readings
      uint16_t validReadings[3];
      int validCount = 0;
      
      if (rtd1 >= 1000 && rtd1 <= 20000) {
        validReadings[validCount++] = rtd1;
      }
      if (rtd2 >= 1000 && rtd2 <= 20000) {
        validReadings[validCount++] = rtd2;
      }
      if (rtd3 >= 1000 && rtd3 <= 20000) {
        validReadings[validCount++] = rtd3;
      }
      
      uint16_t rtd;
      if (validCount == 0) {
        // All readings invalid - show error code instead of bad reading
        // Display as 8XXX where XXX is the average (so we can see it's rejected)
        uint16_t avgBad = (rtd1 + rtd2 + rtd3) / 3;
        currentTempInt = 8000 + (avgBad % 1000); // Show as 8XXX format
        // Don't update rtd, keep showing the error
        return; // Skip beep logic for invalid readings
      } else if (validCount == 1) {
        rtd = validReadings[0];
      } else if (validCount == 2) {
        // Average the two valid readings
        rtd = (validReadings[0] + validReadings[1]) / 2;
      } else {
        // All 3 valid - take median
        if (validReadings[0] > validReadings[1]) {
          if (validReadings[1] > validReadings[2]) rtd = validReadings[1];
          else if (validReadings[0] > validReadings[2]) rtd = validReadings[2];
          else rtd = validReadings[0];
        } else {
          if (validReadings[0] > validReadings[2]) rtd = validReadings[0];
          else if (validReadings[1] > validReadings[2]) rtd = validReadings[2];
          else rtd = validReadings[1];
        }
      }
      
      // Display raw RTD register value directly (0-32768)
      // For PT100 at room temp (~100Ω), expect ~7620
      // 7277 = ~95Ω - this is reasonable! (PT100 at ~20°C)
      // Values < 1000 (< 13Ω) are rejected as invalid
      currentTempInt = rtd;
      
      // Beep when reading changes significantly (quiet and infrequent)
      static uint16_t lastRtd = 0;
      static unsigned long lastChangeBeep = 0;
      if (abs((int)rtd - (int)lastRtd) > 20) { // Smaller threshold to detect changes
        if (millis() - lastChangeBeep > 2000) { // Only beep every 2 seconds max
          tone(PIN_BUZZER, 1200, 15); // Quieter: lower frequency and very short duration
          lastChangeBeep = millis();
        }
        lastRtd = rtd;
      }
    }
  }
  
  // Handle scheduled beep outside of SPI operations to avoid conflicts
  if (beepScheduled) {
    // Small delay after SPI operations before beeping
    delay(20);
    tone(PIN_BUZZER, 800, 30); // Quieter: lower frequency (800Hz) and shorter duration (30ms)
    beepScheduled = false;
    lastBeepTime = millis();
  }
  
  // Update display every 500ms - much less frequent to avoid flickering
  // Only update if we're not showing config register and not reading RTD
  // Don't update display right before/after beep to avoid faltering
  static int lastDisplayedValue = -1;
  if (!showingConfig && millis() - lastUpdate >= 500 && (millis() - lastBeepTime > 100 || lastBeepTime == 0)) {
    lastUpdate = millis();
    
    // Only update display if value actually changed (reduces flickering)
    if (currentTempInt != lastDisplayedValue) {
      displayInt(currentTempInt);
      lastDisplayedValue = currentTempInt;
      delay(10); // Small delay after display update to let SPI settle
    }
  }
  
  delay(50); // Increased delay to reduce SPI bus contention and give more time between operations
}

// --- AUDIO LOGIC ---
void handleAudioLogic(float temp) {
  if (temp < configThreshold) {
    lastBandIndex = -1; 
    return;
  }

  float diff = temp - configThreshold;
  int currentBand = (int)(diff / configStepSize);

  if (currentBand > lastBandIndex) {
    playChirp(true);
    lastBandIndex = currentBand;
  } 
  else if (currentBand < lastBandIndex) {
    if (currentBand >= 0) playChirp(false);
    lastBandIndex = currentBand;
  }
}

void playChirp(bool goingUp) {
  if (goingUp) {
    tone(PIN_BUZZER, 2500, 50); 
    delay(70);
    tone(PIN_BUZZER, 3000, 50);
  } else {
    tone(PIN_BUZZER, 1000, 150); 
  }
}

// --- INPUT HANDLING ---
void handleButtons() {
  bool btnMode = (digitalRead(PIN_BTN_MODE) == LOW);
  bool btnUp   = (digitalRead(PIN_BTN_UP) == LOW);
  bool btnDown = (digitalRead(PIN_BTN_DOWN) == LOW);
  unsigned long now = millis();

  // MODE: Single Click
  static bool lastModeState = false;
  if (btnMode && !lastModeState) {
    if (now - lastInputTime > 200) { 
      cycleMode();
      lastInputTime = now;
    }
  }
  lastModeState = btnMode;

  // UP/DOWN: Hold to scroll
  if (btnUp || btnDown) {
    if (!buttonHeld) {
      modifyValue(btnUp, btnDown);
      buttonHeld = true;
      buttonHoldStart = now;
      lastInputTime = now;
    } else {
      if ((now - buttonHoldStart > BTN_DELAY_INITIAL) && (now - lastInputTime > BTN_DELAY_REPEAT)) {
         modifyValue(btnUp, btnDown);
         lastInputTime = now;
      }
    }
  } else {
    buttonHeld = false;
  }
}

void cycleMode() {
  if (currentMode == MODE_SET_THRESH || currentMode == MODE_SET_STEP) {
    saveSettings();
  }

  if (currentMode == MODE_RUN) currentMode = MODE_SET_THRESH;
  else if (currentMode == MODE_SET_THRESH) currentMode = MODE_SET_STEP;
  else currentMode = MODE_RUN;
  
  lc.clearDisplay(0);
}

void saveSettings() {
  preferences.putFloat("thresh", configThreshold);
  preferences.putFloat("step", configStepSize);
}

void modifyValue(bool up, bool down) {
  float direction = up ? 1.0 : -1.0;

  if (currentMode == MODE_SET_THRESH) {
    configThreshold += (0.5 * direction);
    if (configThreshold < 0) configThreshold = 0;
    if (configThreshold > 500) configThreshold = 500;
  }
  else if (currentMode == MODE_SET_STEP) {
    configStepSize += (0.1 * direction);
    if (configStepSize < 0.1) configStepSize = 0.1;
    if (configStepSize > 10.0) configStepSize = 10.0;
  }
}

// --- DISPLAY HELPERS ---
void displayFloat(float val) {
  // Limit range
  if (val > TEMP_MAX) val = TEMP_MAX;
  if (val < -999.9) val = -999.9;
  
  // Clear display first - blank all digits
  lc.clearDisplay(0);
  delayMicroseconds(100);
  
  bool isNegative = (val < 0);
  if (isNegative) val = -val;
  
  // Multiply by 10 to get integer representation (one decimal place)
  int valToDisplay = (int)(val * 10);
  
  // Extract digits
  // For 77.5°F: valToDisplay = 775
  // d1 = 5 (decimal), d2 = 7 (ones), d3 = 7 (tens), d4 = 0 (hundreds)
  int d1 = valToDisplay % 10;           // Decimal place (rightmost digit)
  int d2 = (valToDisplay / 10) % 10;   // Ones
  int d3 = (valToDisplay / 100) % 10;   // Tens
  int d4 = (valToDisplay / 1000) % 10;  // Hundreds (leftmost)
  
  // Display digits - positions are reversed on this display
  // Position 3 is rightmost, position 0 is leftmost
  // Display format: d4 d3 d2.d1 (e.g., " 77.5")
  // Decimal point goes on the ones digit (d2), not the decimal digit (d1)
  lc.setDigit(0, 3, d1, false);   // Decimal digit (rightmost, no decimal point)
  lc.setDigit(0, 2, d2, true);    // Ones digit (with decimal point)
  lc.setDigit(0, 1, d3, false);   // Tens digit
  lc.setDigit(0, 0, d4, false);   // Hundreds digit (leftmost)
  
  // Handle negative sign (overwrite leftmost digit)
  if (isNegative) {
    lc.setChar(0, 0, '-', false);
  }
}

void displayInt(int val) {
  // Clear display first - blank all digits
  lc.clearDisplay(0);
  delayMicroseconds(100); // Small delay to ensure clear completes
  
  // Extract digits
  int d1 = val % 10;           // Ones (rightmost)
  int d2 = (val / 10) % 10;    // Tens
  int d3 = (val / 100) % 10;   // Hundreds
  int d4 = (val / 1000) % 10;  // Thousands (leftmost)
  
  // Display digits - positions are reversed on this display
  // Position 3 is rightmost, position 0 is leftmost
  // Always set all digits, even if 0 (to ensure clean display)
  lc.setDigit(0, 3, d1, false);  // Ones on right (always show)
  lc.setDigit(0, 2, d2, false);  // Tens
  lc.setDigit(0, 1, d3, false);  // Hundreds
  lc.setDigit(0, 0, d4, false);  // Thousands on left
}