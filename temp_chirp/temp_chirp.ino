/*
 * Column Temperature Monitor - Firmware v1.0
 * Hardware: ESP32-S3, MAX31865 (RTD), MAX7219 (Display), Piezo, Buttons
 * 
 * Functional Description:
 * - Mode 0 (RUN): Displays live Temp (F). Chirps on step crossings.
 * - Mode 1 (SET THRESHOLD): Adjust start point (Default 170.0 F).
 * - Mode 2 (SET GAP): Adjust chirp gap size (Default 0.5 F, Displ: "5").
 */

 #include <SPI.h>
 #include <Adafruit_MAX31865.h>
 #include <LedControl.h>
 
 // --- PIN DEFINITIONS (From Spec Sheet) ---
 // User Inputs
 const int PIN_BTN_MODE = 5;
 const int PIN_BTN_UP   = 6;
 const int PIN_BTN_DOWN = 7;
 const int PIN_BUZZER   = 4;
 
 // SPI Bus (Shared)
 const int PIN_SPI_MOSI = 35;
 const int PIN_SPI_MISO = 37;
 const int PIN_SPI_SCK  = 36;
 
 // Chip Selects
 const int PIN_CS_RTD   = 10;
 const int PIN_CS_DISP  = 11;
 
 // --- CONFIGURATION DEFAULTS ---
 float configThreshold = 170.0; // Starting temp for alerts
 float configStepSize  = 0.5;   // Gap between alerts
 const float R_REF     = 430.0; // Reference Resistor on PCB
 const float R_NOMINAL = 100.0; // PT100
 
 // --- OBJECTS ---
 // RTD Sensor: Software SPI is sometimes more stable with shared buses on ESP32 
 // but we will use Hardware SPI pins defined in constructor for speed.
 Adafruit_MAX31865 thermo = Adafruit_MAX31865(PIN_CS_RTD, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);
 
 // Display: Data, Clk, Load(CS), NumDevices
 LedControl lc = LedControl(PIN_SPI_MOSI, PIN_SPI_SCK, PIN_CS_DISP, 1);
 
 // --- STATE VARIABLES ---
 enum SystemMode { MODE_RUN, MODE_SET_THRESH, MODE_SET_STEP };
 SystemMode currentMode = MODE_RUN;
 
 float currentTempF = 0.0;
 float lastChirpTemp = -999.0; // Track crossing logic
 int   lastBandIndex = -999;
 
 // Button Debounce & Repeat Logic
 unsigned long lastInputTime = 0;
 unsigned long buttonHoldStart = 0;
 bool buttonHeld = false;
 const int BTN_DELAY_INITIAL = 400; // ms before auto-repeat starts
 const int BTN_DELAY_REPEAT  = 100; // ms between repeats
 
 void setup() {
   Serial.begin(115200);
 
   // 1. Init Pins
   pinMode(PIN_BTN_MODE, INPUT_PULLUP); // Using internal pullups
   pinMode(PIN_BTN_UP,   INPUT_PULLUP);
   pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
   pinMode(PIN_BUZZER,   OUTPUT);
 
   // 2. Init Display
   // The MAX7219 might wake up in test mode, turn it off
   lc.shutdown(0, false); 
   lc.setIntensity(0, 8); // Brightness (0-15)
   lc.clearDisplay(0);
 
   // 3. Init RTD
   thermo.begin(MAX31865_3WIRE); 
 
   // Startup Sound
   tone(PIN_BUZZER, 2000, 100);
   delay(150);
   tone(PIN_BUZZER, 3000, 100);
 }
 
 void loop() {
   // --- 1. READ TEMPERATURE (With Smoothing) ---
   uint16_t rtd = thermo.readRTD();
   float ratio = rtd;
   ratio /= 32768;
   float resistance = R_REF * ratio;
   float tempC = thermo.temperature(R_NOMINAL, R_REF);
   
   // Basic error check (unplugged probe usually gives -242 or similar)
   if(thermo.readFault()) {
     thermo.clearFault(); // Reset fault
     // Keep old temp or set to error value, but we'll ignore for simplicity
   } else {
     // Convert to F
     float rawF = (tempC * 9.0 / 5.0) + 32.0;
     // Simple Smoothing (90% old, 10% new) to prevent jitter chirps
     if (currentTempF == 0.0) currentTempF = rawF; // Fast start
     else currentTempF = (currentTempF * 0.9) + (rawF * 0.1);
   }
 
   // --- 2. HANDLE INPUTS ---
   handleButtons();
 
   // --- 3. STATE LOGIC ---
   switch (currentMode) {
     
     case MODE_RUN:
       displayFloat(currentTempF);
       handleAudioLogic(currentTempF);
       break;
 
     case MODE_SET_THRESH:
       // Blink capability could be added, but solid is fine for now
       displayFloat(configThreshold); 
       break;
 
     case MODE_SET_STEP:
       // Requirement: Display "5" for 0.5, "10" for 1.0
       // We display (Step * 10) as an integer
       displayInt((int)(configStepSize * 10));
       break;
   }
   
   delay(20); // Small loop delay
 }
 
 // --- LOGIC FUNCTIONS ---
 
 void handleAudioLogic(float temp) {
   // 1. If we are below threshold, silence and reset band
   if (temp < configThreshold) {
     lastBandIndex = -1;
     return;
   }
 
   // 2. Calculate which "Band" we are in above threshold
   // e.g. Thresh=170, Step=0.5. 
   // Temp=170.2 -> Band 0. 
   // Temp=170.6 -> Band 1.
   float diff = temp - configThreshold;
   int currentBand = (int)(diff / configStepSize);
 
   // 3. Compare with last band
   if (currentBand > lastBandIndex) {
     // We moved UP a band (or crossed threshold)
     playChirp(true);
     lastBandIndex = currentBand;
   } 
   else if (currentBand < lastBandIndex) {
     // We moved DOWN a band
     // Ensure we don't chirp if we just fell below threshold (handled by step 1 check usually, but safety logic:)
     if (currentBand >= 0) {
       playChirp(false);
     }
     lastBandIndex = currentBand;
   }
 }
 
 void playChirp(bool goingUp) {
   if (goingUp) {
     // Rising tone: "Beep-Beep" High
     tone(PIN_BUZZER, 2500, 50);
     delay(70);
     tone(PIN_BUZZER, 3000, 50);
   } else {
     // Falling tone: "Boop" Lower
     tone(PIN_BUZZER, 1000, 150);
   }
 }
 
 void handleButtons() {
   bool btnMode = (digitalRead(PIN_BTN_MODE) == LOW);
   bool btnUp   = (digitalRead(PIN_BTN_UP) == LOW);
   bool btnDown = (digitalRead(PIN_BTN_DOWN) == LOW);
   unsigned long now = millis();
 
   // --- MODE BUTTON (Single Click only) ---
   static bool lastModeState = false;
   if (btnMode && !lastModeState) {
     // Pressed
     if (now - lastInputTime > 200) { // Simple debounce
       cycleMode();
       lastInputTime = now;
     }
   }
   lastModeState = btnMode;
 
   // --- UP / DOWN (With Hold Repeat) ---
   if (btnUp || btnDown) {
     if (!buttonHeld) {
       // First press
       modifyValue(btnUp, btnDown);
       buttonHeld = true;
       buttonHoldStart = now;
       lastInputTime = now;
     } else {
       // Held down
       unsigned long holdTime = now - buttonHoldStart;
       if (holdTime > BTN_DELAY_INITIAL) {
         if (now - lastInputTime > BTN_DELAY_REPEAT) {
            modifyValue(btnUp, btnDown);
            lastInputTime = now;
         }
       }
     }
   } else {
     buttonHeld = false;
   }
 }
 
 void cycleMode() {
   if (currentMode == MODE_RUN) currentMode = MODE_SET_THRESH;
   else if (currentMode == MODE_SET_THRESH) currentMode = MODE_SET_STEP;
   else currentMode = MODE_RUN;
   
   // Visual cue for mode switch
   lc.clearDisplay(0);
 }
 
 void modifyValue(bool up, bool down) {
   float direction = up ? 1.0 : -1.0;
 
   if (currentMode == MODE_SET_THRESH) {
     configThreshold += (0.5 * direction);
     if (configThreshold < 0) configThreshold = 0;
   }
   else if (currentMode == MODE_SET_STEP) {
     // Adjust by 0.1 steps (Displayed as 1)
     configStepSize += (0.1 * direction);
     if (configStepSize < 0.1) configStepSize = 0.1; 
   }
 }
 
 // --- DISPLAY HELPERS ---
 
 // Display a float like "170.5"
 void displayFloat(float val) {
   // MAX7219 only has 4 digits.
   // We'll multiply by 10, cast to int, and turn on the decimal point of digit 1
   
   int valToDisplay = (int)(val * 10); // 170.5 -> 1705
   
   // Extract digits
   int d1 = valToDisplay % 10;        // Tenths
   int d2 = (valToDisplay / 10) % 10; // Ones
   int d3 = (valToDisplay / 100) % 10;// Tens
   int d4 = (valToDisplay / 1000) % 10;// Hundreds
 
   // Digit 0 is far right (Tenths)
   // Digit 1 is Ones (Decimal Point goes here)
   
   lc.setDigit(0, 0, d1, false);
   lc.setDigit(0, 1, d2, true); // true = Decimal Point On
   lc.setDigit(0, 2, d3, false);
   
   // Blank leading zero if less than 1000 (optional, but looks nicer)
   if (d4 == 0) lc.setChar(0, 3, ' ', false); 
   else lc.setDigit(0, 3, d4, false);
 }
 
 // Display an integer (used for the "5", "6", "20" Step settings)
 void displayInt(int val) {
   lc.clearDisplay(0);
   
   int d1 = val % 10;
   int d2 = (val / 10) % 10;
   int d3 = (val / 100) % 10;
   
   lc.setDigit(0, 0, d1, false);
   
   if (val >= 10) lc.setDigit(0, 1, d2, false);
   if (val >= 100) lc.setDigit(0, 2, d3, false);
 }