/*
 * Column Temperature Monitor - Firmware v3.0 (As-Built)
 * Hardware: ESP32-S3-MINI-1 (Custom PCBA)
 * 
 * Changelog:
 * - v3.0: Updated PIN MAPPING to match final manufacturing Netlist.
 *         SPI moved to IO 47/48/45.
 *         CS lines moved to IO 14/21.
 */

#include <SPI.h>
#include <Adafruit_MAX31865.h> 
#include <LedControl.h>        
#include <Preferences.h>       

// --- PIN DEFINITIONS (Derived from Netlist "gge" IDs) ---

// User Inputs
// Netlist says: SW3->IO5, SW4->IO6, SW5->IO7.
// If your physical buttons are swapped, swap these numbers here.
const int PIN_BTN_MODE = 5;  // SW3
const int PIN_BTN_UP   = 6;  // SW4
const int PIN_BTN_DOWN = 7;  // SW5
const int PIN_BUZZER   = 4;  // Q1 Gate

// SPI Bus (Custom Pins for S3-MINI)
// Based on Netlist: Pin 20(SCK), Pin 21(MOSI), Pin 22(MISO)
const int PIN_SPI_SCK  = 47; 
const int PIN_SPI_MOSI = 48; 
const int PIN_SPI_MISO = 45; 

// Chip Selects
// Based on Netlist: Pin 14(CS_RTD), Pin 15(CS_DISP)
const int PIN_CS_RTD   = 14;
const int PIN_CS_DISP  = 21;

// --- CONSTANTS ---
const float R_REF     = 430.0; // Reference Resistor (R4)
const float R_NOMINAL = 100.0; // PT100
const float TEMP_MAX  = 999.0; 

// --- OBJECTS ---
// Software SPI is REQUIRED here because these are not the default VSPI pins
Adafruit_MAX31865 thermo = Adafruit_MAX31865(PIN_CS_RTD, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
LedControl lc = LedControl(PIN_SPI_MOSI, PIN_SPI_SCK, PIN_CS_DISP, 1);
Preferences preferences;

// --- STATE VARIABLES ---
enum SystemMode { MODE_RUN, MODE_SET_THRESH, MODE_SET_STEP };
SystemMode currentMode = MODE_RUN;

float configThreshold = 170.0; 
float configStepSize  = 0.5;   
float currentTempF    = 0.0;
int   lastBandIndex   = -1;

// Input Debounce
unsigned long lastInputTime = 0;
unsigned long buttonHoldStart = 0;
bool buttonHeld = false;
const int BTN_DELAY_INITIAL = 400; 
const int BTN_DELAY_REPEAT  = 100; 

void setup() {
  Serial.begin(115200);

  // 1. Init Pins
  pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BUZZER,   OUTPUT);

  // 2. Load Settings
  preferences.begin("col-mon", false); 
  configThreshold = preferences.getFloat("thresh", 170.0);
  configStepSize  = preferences.getFloat("step", 0.5);

  // 3. Init Display
  // MAX7219 needs a moment to settle on power-up
  delay(100);
  lc.shutdown(0, false); 
  lc.setIntensity(0, 8); 
  lc.clearDisplay(0);

  // 4. Init RTD
  thermo.begin(MAX31865_3WIRE); 

  // 5. Startup Chirp (Confirm Life)
  tone(PIN_BUZZER, 2000, 100);
  delay(150);
  tone(PIN_BUZZER, 3000, 100);
}

void loop() {
  // --- 1. READ TEMPERATURE ---
  uint16_t rtd = thermo.readRTD();
  float tempC = thermo.temperature(R_NOMINAL, R_REF);

  if(thermo.readFault()) {
    thermo.clearFault();
    // Optional: Blink "Err" on display if needed
  } else {
    float rawF = (tempC * 9.0 / 5.0) + 32.0;
    // Smoothing
    if (currentTempF == 0.0) currentTempF = rawF;
    else currentTempF = (currentTempF * 0.9) + (rawF * 0.1);
  }

  // --- 2. INPUTS & LOGIC ---
  handleButtons();

  switch (currentMode) {
    case MODE_RUN:
      displayFloat(currentTempF);
      handleAudioLogic(currentTempF);
      break;

    case MODE_SET_THRESH:
      displayFloat(configThreshold); 
      break;

    case MODE_SET_STEP:
      displayInt((int)(configStepSize * 10));
      break;
  }
  
  delay(20); 
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
  if (val > TEMP_MAX) val = TEMP_MAX;
  
  bool isNegative = (val < 0);
  if (isNegative) val = -val;

  int valToDisplay = (int)(val * 10); 
  
  int d1 = valToDisplay % 10;        
  int d2 = (valToDisplay / 10) % 10; 
  int d3 = (valToDisplay / 100) % 10;
  int d4 = (valToDisplay / 1000) % 10;

  lc.setDigit(0, 0, d1, false);     
  lc.setDigit(0, 1, d2, true);      
  lc.setDigit(0, 2, d3, false);     
  
  if (isNegative) {
     lc.setChar(0, 3, '-', false); 
  } else {
     if (d4 == 0) lc.setChar(0, 3, ' ', false); 
     else lc.setDigit(0, 3, d4, false);
  }
}

void displayInt(int val) {
  lc.clearDisplay(0);
  int d1 = val % 10;
  int d2 = (val / 10) % 10;
  int d3 = (val / 100) % 10;
  
  lc.setDigit(0, 0, d1, false);
  if (val >= 10) lc.setDigit(0, 1, d2, false);
  if (val >= 100) lc.setDigit(0, 2, d3, false);
}