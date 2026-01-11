#ifndef MAX7219DISPLAY_H
#define MAX7219DISPLAY_H

#include "SevenSegmentDisplay.h"
#include <stdint.h>

// Forward declarations
uint8_t charToSegment(char c);
bool isCodeBCompatible(char value);

class MAX7219Display : public SevenSegmentDisplay {
public:
  MAX7219Display(int csPin);
  
  // Implementation of SevenSegmentDisplay interface
  void begin() override;
  void clear() override;
  void setBrightness(uint8_t level) override;
  void displayText(const char* text, bool rightJustify = false) override;
  void displayDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) override;
  void displayTime(uint8_t hours, uint8_t minutes, bool showColon = false, bool hideLeadingZero = false) override;
  void startScrolling(const char* text, unsigned long scrollDelay = 350) override;
  void update() override;
  bool isScrolling() const override;
  void animatePattern(const uint8_t* pattern, size_t patternLen, unsigned long delayMs) override;
  bool isAnimating() const override;

private:
  int csPin;
  uint8_t decodeMask;
  
  struct ScrollState {
    bool active;
    char text[64];
    uint8_t dpMask[64];
    int textLen;
    int scrollPosition;
    unsigned long lastUpdate;
    unsigned long scrollDelay;
  } scrollState;
  
  struct AnimState {
    bool active;
    const uint8_t* pattern;
    size_t patternLen;
    int currentFrame;
    unsigned long lastUpdate;
    unsigned long delayMs;
  } animState;
  
  void writeRegister(uint8_t address, uint8_t value);
  void bitBangWrite(uint8_t address, uint8_t value);
  bool decodeEnabledForDigit(int digit) const;
  bool isCodeBCompatible(char value) const;
  uint8_t raw7seg(char c);
  void setDigitRaw(int digit, int value, bool dp);
  void setCharRaw(int digit, char value, bool dp);
  void writeRawSegment(int digit, uint8_t segments);
  void processScrollingText(const char* text, char* output, uint8_t* dpMask, int* len);
  void renderScrollFrame();
};

#endif // MAX7219DISPLAY_H
