/*
 * MAX7219Display - MAX7219 Implementation
 */

#include "MAX7219Display.h"
#include <SPI.h>
#include <string.h>

// MAX7219 Register definitions
#define REG_DIGIT0      0x01  // Leftmost
#define REG_DIGIT1      0x02
#define REG_DIGIT2      0x03
#define REG_DIGIT3      0x04  // Rightmost
#define REG_DECODE_MODE 0x09
#define REG_INTENSITY   0x0A
#define REG_SCAN_LIMIT  0x0B
#define REG_SHUTDOWN    0x0C
#define REG_TEST        0x0F

MAX7219Display::MAX7219Display(int csPin) : csPin(csPin), decodeMask(0x00) {
  scrollState.active = false;
  scrollState.text[0] = '\0';
  scrollState.textLen = 0;
  scrollState.scrollPosition = 0;
  scrollState.lastUpdate = 0;
  scrollState.scrollDelay = 350;
  
  animState.active = false;
  animState.pattern = nullptr;
  animState.patternLen = 0;
  animState.currentFrame = 0;
  animState.lastUpdate = 0;
  animState.delayMs = 0;
}

void MAX7219Display::writeRegister(uint8_t address, uint8_t value) {
  pinMode(csPin, OUTPUT);
  
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(csPin, LOW);
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
  
  delayMicroseconds(10);
}

void MAX7219Display::bitBangWrite(uint8_t address, uint8_t value) {
  digitalWrite(csPin, LOW);
  delayMicroseconds(10);
  writeRegister(address, value);
}

bool MAX7219Display::decodeEnabledForDigit(int digit) const {
  return (decodeMask & (1 << digit)) != 0;
}

bool MAX7219Display::isCodeBCompatible(char value) const {
  return ::isCodeBCompatible(value);
}

uint8_t MAX7219Display::raw7seg(char c) {
  return ::charToSegment(c);
}

void MAX7219Display::setDigitRaw(int digit, int value, bool dp) {
  if (digit < 0 || digit > 7) return;

  if (decodeEnabledForDigit(digit)) {
    uint8_t code = (uint8_t)(value & 0x0F);
    if (dp) code |= 0x80;
    writeRegister(digit + 1, code);
  } else {
    char c = (char)('0' + (value % 10));
    uint8_t seg = raw7seg(c);
    if (dp) seg |= 0x80;
    writeRegister(digit + 1, seg);
  }
}

void MAX7219Display::setCharRaw(int digit, char value, bool dp) {
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

void MAX7219Display::writeRawSegment(int digit, uint8_t segments) {
  if (digit < 0 || digit > 7) return;
  writeRegister(digit + 1, segments);
}

void MAX7219Display::begin() {
  // CRITICAL: Ensure CS pin is HIGH before starting (matches test_display.ino)
  // This must be done BEFORE any SPI operations to prevent glitches
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  delay(10);
  
  // Initialize MAX7219 - match test_display.ino initDisplay() exactly
  writeRegister(REG_TEST, 0x00);      // Test mode OFF
  delay(10);
  writeRegister(REG_SHUTDOWN, 0x00);  // Shutdown ON
  delay(10);
  writeRegister(REG_SCAN_LIMIT, 0x03); // Scan limit: digits 0..3
  delay(10);
  writeRegister(REG_DECODE_MODE, 0x00); // Decode mode: raw (will be set later as needed)
  decodeMask = 0x00;
  delay(10);
  writeRegister(REG_INTENSITY, 0x08);  // Intensity
  delay(10);
  
  // Clear all digit registers
  for (int i = 1; i <= 4; i++) {
    writeRegister(i, 0x00);
    delay(5);
  }
  
  // Wake up - shutdown mode OFF
  writeRegister(REG_SHUTDOWN, 0x01);
  delay(50);
  
  // Ensure CS pin is HIGH after initialization (defensive)
  digitalWrite(csPin, HIGH);
}

void MAX7219Display::clear() {
  for (int i = 1; i <= 8; i++) {
    int digit = i - 1;
    writeRegister(i, decodeEnabledForDigit(digit) ? 0x0F : 0x00);
  }
}

void MAX7219Display::setBrightness(uint8_t level) {
  if (level > 15) level = 15;
  writeRegister(REG_INTENSITY, level);
}

void MAX7219Display::displayDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
  // Direct digit display - matches test_display.ino showDigits() exactly
  // d0=leftmost, d3=rightmost
  decodeMask = 0x0F;  // Decode mode for all digits
  writeRegister(REG_DECODE_MODE, decodeMask);
  writeRegister(REG_DIGIT0, d0);
  writeRegister(REG_DIGIT1, d1);
  writeRegister(REG_DIGIT2, d2);
  writeRegister(REG_DIGIT3, d3);
}

void MAX7219Display::displayText(const char* text, bool rightJustify) {
  // Stop any active scrolling/animation
  scrollState.active = false;
  animState.active = false;
  
  // Process text to merge dots with previous characters (like IP scrolling)
  char processed[5] = {0};  // Max 4 chars + null terminator
  uint8_t dpMask[4] = {0};  // Decimal point mask for each position
  int processedLen = 0;
  
  for (int i = 0; text[i] != '\0' && processedLen < 4; i++) {
    if (text[i] == '.') {
      // Merge dot with previous character
      if (processedLen > 0) {
        dpMask[processedLen - 1] = 1;
      }
    } else {
      processed[processedLen++] = text[i];
    }
  }
  
  // Determine decode mode based on content
  bool needsRaw = false;
  for (int i = 0; i < processedLen && i < 4; i++) {
    if (!isCodeBCompatible(processed[i])) {
      needsRaw = true;
      break;
    }
  }
  
  // Set decode mode
  decodeMask = needsRaw ? 0x00 : 0x0F;
  writeRegister(REG_DECODE_MODE, decodeMask);
  
  // Calculate display start position for right-justification
  int displayStart = 0;
  if (rightJustify && processedLen < 4) {
    displayStart = 4 - processedLen;  // Start displaying from this position (leaves spaces on left)
  }
  
  // Display up to 4 characters with decimal points
  for (int i = 0; i < 4; i++) {
    bool showDP = false;
    int sourcePos = i - displayStart;  // Position in processed array
    
    if (sourcePos >= 0 && sourcePos < processedLen) {
      // Check if this position should have a decimal point
      if (dpMask[sourcePos]) {
        showDP = true;
      }
      setCharRaw(i, processed[sourcePos], showDP);
    } else {
      setCharRaw(i, ' ', false);
    }
  }
}

void MAX7219Display::displayTime(uint8_t hours, uint8_t minutes, bool showColon, bool hideLeadingZero) {
  // Stop any active scrolling/animation
  scrollState.active = false;
  animState.active = false;
  
  // Convert to 12-hour format if needed
  if (hideLeadingZero) {
    if (hours == 0) hours = 12;
    else if (hours > 12) hours -= 12;
  }
  
  // Format as HHMM
  int displayValue = (hours * 100) + minutes;
  int d1 = displayValue % 10;           // Minutes ones (rightmost)
  int d2 = (displayValue / 10) % 10;   // Minutes tens
  int d3 = (displayValue / 100) % 10;  // Hours ones
  int d4 = (displayValue / 1000) % 10; // Hours tens (leftmost)
  
  // Set decode mode for digits - match test_display.ino showDigits() exactly
  decodeMask = 0x0F;
  writeRegister(REG_DECODE_MODE, decodeMask);
  delay(10);  // Small delay like test_display has between register writes
  
  // Display time - DIGIT0 = leftmost, DIGIT3 = rightmost
  // Use direct register writes like test_display.ino showDigits() for reliability
  if (hideLeadingZero && d4 == 0) {
    writeRegister(REG_DIGIT0, 0x0F);  // Blank (0x0F = blank in decode mode)
  } else {
    writeRegister(REG_DIGIT0, d4);   // Hours tens (leftmost, DIGIT0)
  }
  delay(5);
  uint8_t d3Value = d3;
  if (showColon) d3Value |= 0x80;  // Add decimal point for colon
  writeRegister(REG_DIGIT1, d3Value); // Hours ones + colon (DIGIT1)
  delay(5);
  writeRegister(REG_DIGIT2, d2);     // Minutes tens (DIGIT2)
  delay(5);
  writeRegister(REG_DIGIT3, d1);     // Minutes ones (rightmost, DIGIT3)
  delay(5);
}

void MAX7219Display::processScrollingText(const char* text, char* output, uint8_t* dpMask, int* len) {
  *len = 0;
  memset(dpMask, 0, 64);
  
  // Add leading spaces
  for (int i = 0; i < 4; i++) {
    output[(*len)++] = ' ';
  }
  
  // Copy text, converting dots to decimal points on previous digit
  for (int i = 0; text[i] != '\0' && *len < 60; i++) {
    if (text[i] == '.') {
      if (*len > 0) {
        dpMask[*len - 1] = 1;  // Add DP to previous char
      }
    } else {
      output[(*len)++] = text[i];
    }
  }
  
  // Add trailing spaces
  for (int i = 0; i < 4; i++) {
    output[(*len)++] = ' ';
  }
  output[*len] = '\0';
}

void MAX7219Display::renderScrollFrame() {
  // Display 4 characters starting at scrollPosition
  // DIGIT0 = leftmost, DIGIT3 = rightmost
  for (int i = 0; i < 4; i++) {
    int strPos = scrollState.scrollPosition + i;
    uint8_t seg = 0x00;
    
    if (strPos >= 0 && strPos < scrollState.textLen) {
      seg = charToSegment(scrollState.text[strPos]);
      if (scrollState.dpMask[strPos]) {
        seg |= 0x80;  // Add decimal point
      }
    }
    
    // DIGIT0 = leftmost, so first char goes to DIGIT0
    writeRawSegment(i, seg);
  }
}

void MAX7219Display::startScrolling(const char* text, unsigned long scrollDelay) {
  scrollState.active = true;
  scrollState.scrollDelay = scrollDelay;
  scrollState.scrollPosition = 0;
  scrollState.lastUpdate = millis();
  
  // Process text (handle dots, add padding)
  processScrollingText(text, scrollState.text, scrollState.dpMask, &scrollState.textLen);
  
  // Set decode mode to raw for scrolling
  decodeMask = 0x00;
  writeRegister(REG_DECODE_MODE, decodeMask);
  
  // Render first frame
  renderScrollFrame();
}

void MAX7219Display::update() {
  unsigned long now = millis();
  
  // Update scrolling
  if (scrollState.active) {
    if (now - scrollState.lastUpdate >= scrollState.scrollDelay) {
      scrollState.lastUpdate = now;
      renderScrollFrame();
      
      scrollState.scrollPosition++;
      if (scrollState.scrollPosition > scrollState.textLen - 4) {
        scrollState.scrollPosition = 0;  // Reset scroll position
      }
    }
  }
  
  // Update animation
  if (animState.active && animState.pattern != nullptr) {
    if (now - animState.lastUpdate >= animState.delayMs) {
      animState.lastUpdate = now;
      
      // Display current pattern frame
      for (int i = 0; i < 4 && i < (int)animState.patternLen; i++) {
        writeRawSegment(i, animState.pattern[animState.currentFrame * 4 + i]);
      }
      
      animState.currentFrame++;
      if (animState.currentFrame >= animState.patternLen / 4) {
        animState.currentFrame = 0;  // Loop animation
      }
    }
  }
}

bool MAX7219Display::isScrolling() const {
  return scrollState.active;
}

void MAX7219Display::animatePattern(const uint8_t* pattern, size_t patternLen, unsigned long delayMs) {
  animState.active = true;
  animState.pattern = pattern;
  animState.patternLen = patternLen;
  animState.currentFrame = 0;
  animState.lastUpdate = millis();
  animState.delayMs = delayMs;
  
  // Stop scrolling
  scrollState.active = false;
  
  // Set decode mode to raw for patterns
  decodeMask = 0x00;
  writeRegister(REG_DECODE_MODE, decodeMask);
}

bool MAX7219Display::isAnimating() const {
  return animState.active;
}
