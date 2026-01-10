/*
 * MAX7219 Display Test v3 - Correct digit order + right-to-left scroll
 * Hardware: ESP32-S3-MINI-1 + MAX7219CNG
 * 
 * Digit mapping (confirmed from test):
 *   DIGIT0 = leftmost on display
 *   DIGIT3 = rightmost on display
 */

 #include <SPI.h>
 #include <WiFi.h>
 
 // Pin definitions
 const int PIN_CS_DISP  = 11;
 const int PIN_SPI_SCK  = 16;
 const int PIN_SPI_MOSI = 17;
 const int PIN_SPI_MISO = 18;
 const int PIN_BUZZER   = 4;
 
 // MAX7219 Registers
 #define REG_DIGIT0      0x01  // Leftmost
 #define REG_DIGIT1      0x02
 #define REG_DIGIT2      0x03
 #define REG_DIGIT3      0x04  // Rightmost
 #define REG_DECODE      0x09
 #define REG_INTENSITY   0x0A
 #define REG_SCANLIMIT   0x0B
 #define REG_SHUTDOWN    0x0C
 #define REG_TEST        0x0F
 
 // WiFi credentials
 const char* wifiSSID = "salty";
 const char* wifiPassword = "I4getit2";
 
 void hwSpiSend(uint8_t address, uint8_t data) {
   SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
   digitalWrite(PIN_CS_DISP, LOW);
   SPI.transfer(address);
   SPI.transfer(data);
   digitalWrite(PIN_CS_DISP, HIGH);
   SPI.endTransaction();
 }
 
 void initDisplay() {
   hwSpiSend(REG_TEST, 0x00);
   delay(10);
   hwSpiSend(REG_SHUTDOWN, 0x00);
   delay(10);
   hwSpiSend(REG_SCANLIMIT, 0x03);
   delay(10);
   hwSpiSend(REG_DECODE, 0x00);  // Raw mode
   delay(10);
   hwSpiSend(REG_INTENSITY, 0x08);
   delay(10);
   for (int i = 1; i <= 4; i++) {
     hwSpiSend(i, 0x00);
     delay(5);
   }
   hwSpiSend(REG_SHUTDOWN, 0x01);
   delay(50);
 }
 
 // Raw 7-segment patterns
 // MAX7219 no-decode: bit0=A, bit1=B, bit2=C, bit3=D, bit4=E, bit5=F, bit6=G, bit7=DP
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
 
 void showDigits(int d0, int d1, int d2, int d3) {
   // d0=leftmost, d3=rightmost
   hwSpiSend(REG_DECODE, 0x0F);  // Decode mode for simple digits
   hwSpiSend(REG_DIGIT0, d0);
   hwSpiSend(REG_DIGIT1, d1);
   hwSpiSend(REG_DIGIT2, d2);
   hwSpiSend(REG_DIGIT3, d3);
 }
 
 void setup() {
   Serial.begin(115200);
   delay(2000);
   
   pinMode(PIN_BUZZER, OUTPUT);
   pinMode(PIN_CS_DISP, OUTPUT);
   digitalWrite(PIN_CS_DISP, HIGH);
   
   Serial.println("\n========================================");
   Serial.println("MAX7219 Test v3 - Right-to-left scroll");
   Serial.println("========================================\n");
   
   // Init WiFi first
   Serial.println("Connecting to WiFi...");
   WiFi.mode(WIFI_STA);
   WiFi.begin(wifiSSID, wifiPassword);
   
   int attempts = 0;
   while (WiFi.status() != WL_CONNECTED && attempts < 30) {
     delay(500);
     Serial.print(".");
     attempts++;
   }
   
   if (WiFi.status() == WL_CONNECTED) {
     Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
     tone(PIN_BUZZER, 2000, 100);
   } else {
     Serial.println("\nWiFi failed!");
   }
   
   // Init SPI and display
   SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
   delay(100);
   initDisplay();
   
   // Confirm digit positions: show "1234" (should read 1234 left-to-right)
   Serial.println("Showing 1234 (confirming digit order)...");
   showDigits(1, 2, 3, 4);  // Left to right: 1, 2, 3, 4
   tone(PIN_BUZZER, 1000, 100);
   delay(3000);
   
   Serial.println("Starting IP scroll (right-to-left, like news ticker)...");
 }
 
 void loop() {
   static unsigned long lastScrollUpdate = 0;
   static int scrollPos = 0;
   static char displayStr[24];  // Processed string (dots merged with prev digit)
   static uint8_t dpMask[24];   // Which positions have decimal points
   static int displayLen = 0;
   static bool initialized = false;
   
   if (WiFi.status() != WL_CONNECTED) {
     if (millis() - lastScrollUpdate >= 1000) {
       lastScrollUpdate = millis();
       hwSpiSend(REG_DECODE, 0x0F);
       hwSpiSend(REG_DIGIT0, 0x0A);
       hwSpiSend(REG_DIGIT1, 0x0A);
       hwSpiSend(REG_DIGIT2, 0x0A);
       hwSpiSend(REG_DIGIT3, 0x0A);
     }
     return;
   }
   
   // Build display string once (remove dots, mark DP positions)
   if (!initialized) {
     IPAddress ip = WiFi.localIP();
     char ipStr[20];
     sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
     
     // Process: copy chars, skip dots but mark previous char for DP
     displayLen = 0;
     memset(dpMask, 0, sizeof(dpMask));
     
     // Add leading spaces
     for (int i = 0; i < 4; i++) {
       displayStr[displayLen++] = ' ';
     }
     
     // Copy IP, merging dots
     for (int i = 0; ipStr[i] != '\0'; i++) {
       if (ipStr[i] == '.') {
         if (displayLen > 0) {
           dpMask[displayLen - 1] = 1;  // Add DP to previous char
         }
       } else {
         displayStr[displayLen++] = ipStr[i];
       }
     }
     
     // Add trailing spaces
     for (int i = 0; i < 4; i++) {
       displayStr[displayLen++] = ' ';
     }
     displayStr[displayLen] = '\0';
     
     Serial.print("Display string: [");
     for (int i = 0; i < displayLen; i++) {
       Serial.print(displayStr[i] == ' ' ? '_' : displayStr[i]);
       if (dpMask[i]) Serial.print('.');
     }
     Serial.println("]");
     
     initialized = true;
   }
   
   // Scroll
   if (millis() - lastScrollUpdate >= 350) {
     lastScrollUpdate = millis();
     
     hwSpiSend(REG_DECODE, 0x00);  // Raw mode
     
     // Display 4 characters starting at scrollPos
     for (int i = 0; i < 4; i++) {
       int strPos = scrollPos + i;
       uint8_t seg = 0x00;
       
       if (strPos >= 0 && strPos < displayLen) {
         seg = charToSegment(displayStr[strPos]);
         if (dpMask[strPos]) {
           seg |= 0x80;  // Add decimal point
         }
       }
       
       // DIGIT0 = leftmost, so first char goes to DIGIT0
       hwSpiSend(REG_DIGIT0 + i, seg);
     }
     
     scrollPos++;
     if (scrollPos > displayLen - 4) {
       scrollPos = 0;
     }
   }
 }
 