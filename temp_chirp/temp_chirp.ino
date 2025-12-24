/*
 * Column Temperature Monitor - Firmware v2.0 (Production)
 * Hardware: ESP32-S3-MINI-1
 * 
 * Changelog:
 * - v2.0: Added Persistence (Preferences), Fixed Display Overflow, Validated Pins
 */

 #include <SPI.h>
 #include <Adafruit_MAX31865.h> // Library: "Adafruit MAX31865"
 #include <LedControl.h>        // Library: "LedControl" by Eberhard Fahle
 #include <Preferences.h>       // Native ESP32 Library for saving settings
 
 // --- PIN DEFINITIONS (Matched to v1.0 Spec) ---
 // Note: We use Software SPI (Bit-Banging) because LedControl 
 // requires it. This allows us to share pins 35/36/37 easily.
 
 // User Inputs
 const int PIN_BTN_MODE = 5;
 const int PIN_BTN_UP   = 6;
 const int PIN_BTN_DOWN = 7;
 const int PIN_BUZZER   = 4;
 
 // Shared SPI Bus (Software SPI)
 const int PIN_SPI_MOSI = 35; 
 const int PIN_SPI_MISO = 37; 
 const int PIN_SPI_SCK  = 36; 
 
 // Chip Selects
 const int PIN_CS_RTD   = 10;
 const int PIN_CS_DISP  = 11;
 
 // --- CONSTANTS ---
 const float R_REF     = 430.0; // Value of Reference Resistor on PCB (430 ohm)
 const float R_NOMINAL = 100.0; // PT100 Nominal
 const float TEMP_MAX  = 999.0; // Display cap
 
 // --- OBJECTS ---
 // 1. RTD Sensor: Initialized with 4 pins = Software SPI
 Adafruit_MAX31865 thermo = Adafruit_MAX31865(PIN_CS_RTD, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
 
 // 2. Display: Data(MOSI), Clk(SCK), Load(CS) = Software SPI
 LedControl lc = LedControl(PIN_SPI_MOSI, PIN_SPI_SCK, PIN_CS_DISP, 1);
 
 // 3. Preferences (Non-volatile storage)
 Preferences preferences;
 
 // --- STATE VARIABLES ---
 enum SystemMode { MODE_RUN, MODE_SET_THRESH, MODE_SET_STEP };
 SystemMode currentMode = MODE_RUN;
 
 float configThreshold = 170.0; // Default (will be overwritten by storage)
 float configStepSize  = 0.5;   // Default
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
 
   // 1. Load Settings from Flash
   preferences.begin("col-mon", false); // Namespace "col-mon", Read/Write
   configThreshold = preferences.getFloat("thresh", 170.0);
   configStepSize  = preferences.getFloat("step", 0.5);
 
   // 2. Init Pins
   pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
   pinMode(PIN_BTN_UP,   INPUT_PULLUP);
   pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
   pinMode(PIN_BUZZER,   OUTPUT);
 
   // 3. Init Display
   lc.shutdown(0, false); // Wake up
   lc.setIntensity(0, 8); // Brightness 0-15
   lc.clearDisplay(0);
 
   // 4. Init RTD
   thermo.begin(MAX31865_3WIRE); 
 
   // Startup Chirp
   tone(PIN_BUZZER, 2000, 100);
 }
 
 void loop() {
   // --- 1. READ TEMPERATURE ---
   uint16_t rtd = thermo.readRTD();
   float tempC = thermo.temperature(R_NOMINAL, R_REF);
 
   // Check for hardware faults (Cable cut, etc)
   if(thermo.readFault()) {
     thermo.clearFault();
     // In production, you might display "Err", 
     // but for now we just hold the last valid temp.
   } else {
     float rawF = (tempC * 9.0 / 5.0) + 32.0;
     
     // Smooth filter (Weighted Average)
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
       // Display 0.5 as " 5", 1.0 as "10"
       displayInt((int)(configStepSize * 10));
       break;
   }
   
   delay(20); 
 }
 
 // --- AUDIO LOGIC ---
 void handleAudioLogic(float temp) {
   if (temp < configThreshold) {
     lastBandIndex = -1; // Reset when below threshold
     return;
   }
 
   // Calculate "Band" (How many steps above threshold)
   float diff = temp - configThreshold;
   int currentBand = (int)(diff / configStepSize);
 
   if (currentBand > lastBandIndex) {
     // Moved Up a band
     playChirp(true);
     lastBandIndex = currentBand;
   } 
   else if (currentBand < lastBandIndex) {
     // Moved Down a band
     if (currentBand >= 0) playChirp(false);
     lastBandIndex = currentBand;
   }
 }
 
 void playChirp(bool goingUp) {
   if (goingUp) {
     tone(PIN_BUZZER, 2500, 50); // High-High
     delay(70);
     tone(PIN_BUZZER, 3000, 50);
   } else {
     tone(PIN_BUZZER, 1000, 150); // Low
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
   // If we are leaving a SET mode, Save to Flash
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
     // Clamp limits
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
   // Safety Cap
   if (val > TEMP_MAX) val = TEMP_MAX;
   
   // Handle Negatives (Basic)
   bool isNegative = (val < 0);
   if (isNegative) val = -val;
 
   int valToDisplay = (int)(val * 10); // 170.5 -> 1705
   
   // Extract Digits
   int d1 = valToDisplay % 10;        
   int d2 = (valToDisplay / 10) % 10; 
   int d3 = (valToDisplay / 100) % 10;
   int d4 = (valToDisplay / 1000) % 10;
 
   lc.setDigit(0, 0, d1, false);     // Tenths
   lc.setDigit(0, 1, d2, true);      // Ones + Decimal Point
   lc.setDigit(0, 2, d3, false);     // Tens
   
   // Hundreds digit handling
   if (isNegative) {
      lc.setChar(0, 3, '-', false); // Show minus sign
   } else {
      if (d4 == 0) lc.setChar(0, 3, ' ', false); // Blank leading zero
      else lc.setDigit(0, 3, d4, false);
   }
 }
 
 void displayInt(int val) {
   lc.clearDisplay(0);
   // Displays integers right-aligned (Used for Step Size config)
   int d1 = val % 10;
   int d2 = (val / 10) % 10;
   int d3 = (val / 100) % 10;
   
   lc.setDigit(0, 0, d1, false);
   if (val >= 10) lc.setDigit(0, 1, d2, false);
   if (val >= 100) lc.setDigit(0, 2, d3, false);
 }