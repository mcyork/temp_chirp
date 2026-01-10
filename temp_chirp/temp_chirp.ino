/*
 * Column Temperature Monitor - Firmware v3.2 (Force Read)
 * Hardware: ESP32-S3-MINI-1
 * 
 * FIXES:
 * 1. REMOVED the "if (fault)" block that was preventing updates.
 * 2. Added startup "Raw Resistance" display to verify wiring.
 *    - If you see ~100 to 110 at boot, sensor is good.
 *    - If you see 0, sensor is disconnected/shorted.
 * 3. Forces temperature display even if sensor complains.
 */

 #include <SPI.h>
 #include <Adafruit_MAX31865.h> 
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
 const int PIN_CS_RTD   = 10;
 const int PIN_CS_DISP  = 11;
 
 // --- CONSTANTS ---
 const float R_REF     = 430.0; // Reference Resistor (R4 on PT100 boards)
 const float R_NOMINAL = 100.0; // PT100
 const float TEMP_MAX  = 999.0; 
 
 // --- SIMPLE MAX7219 DRIVER (Display) ---
 class SimpleMAX7219 {
 private:
   int csPin;
   void writeRegister(uint8_t address, uint8_t value) {
     // Ensure we strictly use SPI MODE 0 for Display
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
 };
 
 // --- OBJECTS ---
 // PT100 Sensor Object
 Adafruit_MAX31865 thermo = Adafruit_MAX31865(PIN_CS_RTD);
 // Display Object
 SimpleMAX7219 lc(PIN_CS_DISP);
 Preferences preferences;
 
 // --- STATE VARIABLES ---
 enum SystemMode { MODE_RUN, MODE_SET_THRESH, MODE_SET_STEP };
 SystemMode currentMode = MODE_RUN;
 
 float configThreshold = 170.0; 
 float configStepSize  = 0.5;   
 float currentTempC    = 0.0;
 int   lastBandIndex   = -1;
 
 unsigned long lastInputTime = 0;
 bool buttonHeld = false;
 unsigned long buttonHoldStart = 0;
 
 // Forward Declarations
 void displayFloat(float val);
 void displayInt(int val);
 void handleButtons();
 void handleAudioLogic(float temp);
 void playChirp(bool goingUp);
 void cycleMode();
 void saveSettings();
 void modifyValue(bool up, bool down);
 
 void setup() {
   // 1. Init Pins
   pinMode(PIN_BTN_MODE, INPUT_PULLUP); 
   pinMode(PIN_BTN_UP,   INPUT_PULLUP);
   pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
   pinMode(PIN_BUZZER,   OUTPUT);
   
   // Set CS High to prevent bus conflict
   pinMode(PIN_CS_RTD, OUTPUT);
   digitalWrite(PIN_CS_RTD, HIGH);
   pinMode(PIN_CS_DISP, OUTPUT);
   digitalWrite(PIN_CS_DISP, HIGH);
 
   // 2. Init SPI
   SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
   delay(100);
   
   // 3. Init Display
   lc.begin();
   
   // 4. Init Sensor (Try 3-Wire config)
   thermo.begin(MAX31865_3WIRE); 
   
   // --- DEBUG STARTUP SEQUENCE ---
   // Show Raw Resistance for 2 seconds to verify wiring.
   // Expect ~100.0 to 110.0 for PT100.
   // If 0.0 or >400, wiring is wrong.
   uint16_t rtd = thermo.readRTD();
   float ohms = thermo.temperature(R_NOMINAL, R_REF) * 0.0; // Dummy calc to init
   ohms = (float)rtd; 
   // Actually calculate ohms:
   ohms = ((float)rtd * R_REF) / 32768.0;
   
   displayFloat(ohms); 
   tone(PIN_BUZZER, 2000, 100);
   delay(2000); 
   // ------------------------------
 
   // Load Prefs
   preferences.begin("col_temp", false);
   configThreshold = preferences.getFloat("thresh", 170.0);
   configStepSize  = preferences.getFloat("step", 0.5);
 }
 
 void loop() {
   handleButtons();
 
   if (currentMode == MODE_RUN) {
     static unsigned long lastTempRead = 0;
     
     // Read every 200ms
     if (millis() - lastTempRead >= 200) {
       lastTempRead = millis();
       
       // 1. Clear any stuck faults so we can read fresh
       uint8_t fault = thermo.readFault();
       if (fault) {
         thermo.clearFault();
       }
       
       // 2. Force Read Temperature
       // We do not put this in an 'else'. We read no matter what.
       float tempVal = thermo.temperature(R_NOMINAL, R_REF);
       
       // 3. Update Global
       currentTempC = tempVal;
       
       handleAudioLogic(currentTempC);
     }
     
     // Check for catastrophic failure (0 ohms = ~ -242C)
     if (currentTempC < -200) {
       lc.setChar(0, 0, 'E', false);
       lc.setChar(0, 1, 'r', false);
       lc.setChar(0, 2, 'r', false);
       lc.setChar(0, 3, ' ', false);
     } else {
       displayFloat(currentTempC);
     }
   } 
   else if (currentMode == MODE_SET_THRESH) {
     displayFloat(configThreshold); 
   } 
   else if (currentMode == MODE_SET_STEP) {
     displayFloat(configStepSize);
   }
   
   delay(10);
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
     tone(PIN_BUZZER, 2500, 50); delay(70); tone(PIN_BUZZER, 3000, 50);
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
 
   static bool lastModeBtnState = false;
   if (btnMode && !lastModeBtnState) {
     if (now - lastInputTime > 200) { 
       cycleMode();
       tone(PIN_BUZZER, 2000, 50);
       lastInputTime = now;
     }
   }
   lastModeBtnState = btnMode;
 
   if (btnUp || btnDown) {
     if (!buttonHeld) {
       modifyValue(btnUp, btnDown);
       buttonHeld = true;
       buttonHoldStart = now;
       lastInputTime = now;
     } else {
       if ((now - buttonHoldStart > 400) && (now - lastInputTime > 100)) {
          modifyValue(btnUp, btnDown);
          lastInputTime = now;
       }
     }
   } else {
     buttonHeld = false;
   }
 }
 
 void cycleMode() {
   if (currentMode != MODE_RUN) saveSettings();
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
   // Range Limit
   if (val > 999.9) val = 999.9;
   if (val < -99.9) val = -99.9;
   
   bool isNegative = (val < 0);
   if (isNegative) val = -val;
   
   int valToDisplay = (int)(val * 10 + 0.5); 
   
   int d1 = valToDisplay % 10;           
   int d2 = (valToDisplay / 10) % 10;    
   int d3 = (valToDisplay / 100) % 10;   
   int d4 = (valToDisplay / 1000) % 10;  
   
   lc.setDigit(0, 3, d1, false);   // Decimal (Rightmost)
   lc.setDigit(0, 2, d2, true);    // Ones + Dot
   
   // Clean zero handling
   if (valToDisplay < 100) lc.setChar(0, 1, ' ', false);
   else lc.setDigit(0, 1, d3, false);
   
   if (valToDisplay < 1000) {
      if (isNegative) lc.setChar(0, 0, '-', false);
      else lc.setChar(0, 0, ' ', false);
   } else {
      lc.setDigit(0, 0, d4, false);
   }
 }