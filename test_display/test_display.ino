/*
 * MAX7219 Display Test - Using Library
 * Hardware: ESP32-S3-MINI-1 + MAX7219CNG
 * 
 * This test uses the SevenSegmentDisplay library instead of direct SPI calls
 * to see if the library works in the same setup() flow that works with direct calls.
 */

#include <SPI.h>
#include <WiFi.h>
#include "../NTP_Clock/SevenSegmentDisplay/MAX7219Display.h"
#include "../NTP_Clock/SevenSegmentDisplay/MAX7219Display.cpp"

// Pin definitions
const int PIN_CS_DISP  = 11;
const int PIN_SPI_SCK  = 16;
const int PIN_SPI_MOSI = 17;
const int PIN_SPI_MISO = 18;
const int PIN_BUZZER   = 4;

// WiFi credentials
const char* wifiSSID = "salty";
const char* wifiPassword = "I4getit2";

MAX7219Display display(PIN_CS_DISP);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_CS_DISP, OUTPUT);
  digitalWrite(PIN_CS_DISP, HIGH);
  
  Serial.println("\n========================================");
  Serial.println("MAX7219 Test - Using Library");
  Serial.println("========================================\n");
  
  // Init WiFi FIRST (exactly like working test)
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
  
  // SPI and display AFTER WiFi (exactly like working test)
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  delay(100);
  
  display.begin();
  
  // Use library instead of showDigits
  Serial.println("Testing library displayText(\"1234\")...");
  display.displayText("1234");
  tone(PIN_BUZZER, 1000, 100);
  delay(3000);
  
  Serial.println("Starting IP scroll with library...");
}

void loop() {
  static bool ipScrollingStarted = false;
  
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  if (!ipScrollingStarted) {
    IPAddress ip = WiFi.localIP();
    char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    display.startScrolling(ipStr, 350);
    ipScrollingStarted = true;
    Serial.println("IP scrolling started: " + String(ipStr));
  }
  
  // Update scrolling (library handles the timing)
  display.update();
}
