/*
 * SevenSegmentDisplay - Abstract Base Class
 * 
 * Abstract interface for 7-segment display drivers.
 * Allows swapping between different display chips (MAX7219, TM1637, HT16K33, etc.)
 */

#ifndef SEVENSEGMENTDISPLAY_H
#define SEVENSEGMENTDISPLAY_H

#include <stdint.h>

class SevenSegmentDisplay {
public:
  virtual ~SevenSegmentDisplay() {}
  
  // Initialization
  virtual void begin() = 0;
  virtual void clear() = 0;
  virtual void setBrightness(uint8_t level) = 0;
  
  // Generic text display - handles justification, formatting automatically
  // rightJustify: if true, right-aligns text (e.g., "  2.14"), if false left-aligns (e.g., "AP  ")
  virtual void displayText(const char* text, bool rightJustify = false) = 0;
  
  virtual void displayDigits(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) = 0;
  
  // Time display - formatted HHMM with optional colon
  // hours: 0-23 (will be converted to 12-hour if hideLeadingZero is true)
  // minutes: 0-59
  // showColon: blink decimal point on 100s digit as colon
  // hideLeadingZero: suppress leading zero in 12-hour mode (e.g., " 1:23" instead of "01:23")
  virtual void displayTime(uint8_t hours, uint8_t minutes, bool showColon = false, bool hideLeadingZero = false) = 0;
  
  // Generic scrolling - works for any text (IP addresses, messages, etc.)
  // Automatically handles dot-to-decimal-point conversion for IP addresses
  // text: string to scroll (e.g., "192.168.4.1" or "Hello World")
  // scrollDelay: milliseconds between scroll steps (default 350ms)
  virtual void startScrolling(const char* text, unsigned long scrollDelay = 350) = 0;
  
  // Update scrolling/animation state - call from loop() regularly
  virtual void update() = 0;
  
  // Check if currently scrolling
  virtual bool isScrolling() const = 0;
  
  // Pattern animation - custom segment patterns
  // pattern: array of segment patterns (one per digit)
  // patternLen: number of patterns in array
  // delayMs: milliseconds between pattern changes
  virtual void animatePattern(const uint8_t* pattern, size_t patternLen, unsigned long delayMs) = 0;
  
  // Check if currently animating
  virtual bool isAnimating() const = 0;
};

#endif // SEVENSEGMENTDISPLAY_H
