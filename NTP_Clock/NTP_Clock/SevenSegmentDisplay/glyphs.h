#ifndef GLYPHS_H
#define GLYPHS_H

#include <stdint.h>

// Segments are REVERSED:  bit0->G, bit1->F, bit2->E, bit3->D, bit4->C, bit5->B, bit6->A, bit7->DP
// To display segment A (top), send bit6. To display B (top-right), send bit5. etc.
// This matches the physical display wiring

inline uint8_t charToSegment(char c) {
  switch (c) {
    // digits
    case '0': return 0x7E; // A B C D E F
    case '1': return 0x30; // B C
    case '2': return 0x6D; // A B D E G
    case '3': return 0x79; // A B C D G
    case '4': return 0x33; // B C F G
    case '5': return 0x5B; // A C D F G
    case '6': return 0x5F; // A C D E F G
    case '7': return 0x70; // A B C
    case '8': return 0x7F; // A B C D E F G
    case '9': return 0x7B; // A B C D F G

    // letters - Segments are REVERSED: bit0->G, bit1->F, bit2->E, bit3->D, bit4->C, bit5->B, bit6->A, bit7->DP
    case 'A': case 'a': return 0x77; // A B C E F G -> bit6|bit5|bit4|bit2|bit1|bit0 = 0x40|0x20|0x10|0x04|0x02|0x01 = 0x77
    case 'B': case 'b': return 0x7C; // C D E F G
    case 'C': case 'c': return 0x4E; // A D E F -> bit6|bit3|bit2|bit1 = 0x40|0x08|0x04|0x02 = 0x4E
    case 'D': case 'd': return 0x5E; // B C D E G
    case 'E': case 'e': return 0x4F; // A D E F G -> bit6|bit3|bit2|bit1|bit0 = 0x40|0x08|0x04|0x02|0x01 = 0x4F
    case 'F': case 'f': return 0x47; // A E F G
    case 'G': case 'g': return 0x5E; // A C D E F
    case 'H': case 'h': return 0x37; // B C E F G -> bit5|bit4|bit2|bit1|bit0 = 0x20|0x10|0x04|0x02|0x01 = 0x37
    case 'I': case 'i': return 0x30; // B C
    case 'J': case 'j': return 0x3C; // B C D E
    case 'K': case 'k': return 0x37; // B C E F G (same as H)
    case 'L': case 'l': return 0x0E; // D E F -> bit3|bit2|bit1 = 0x08|0x04|0x02 = 0x0E
    case 'M': case 'm': return 0x77; // A B C E F G (same as A)
    case 'N': case 'n': return 0x15; // C E G -> bit4|bit2|bit0 = 0x10|0x04|0x01 = 0x15
    case 'O': case 'o': return 0x1D; // C D E G -> bit4|bit3|bit2|bit0 = 0x10|0x08|0x04|0x01 = 0x1D
    case 'P': case 'p': return 0x67; // A B E F G -> bit6|bit5|bit2|bit1|bit0 = 0x40|0x20|0x04|0x02|0x01 = 0x67
    case 'Q': case 'q': return 0x73; // A B C F G
    case 'R': case 'r': return 0x05; // E G -> bit2|bit0 = 0x04|0x01 = 0x05
    case 'S': case 's': return 0x5B; // A C D F G (same as 5)
    case 'T': case 't': return 0x0F; // D E F G
    case 'U': case 'u': return 0x3E; // B C D E F
    case 'V': case 'v': return 0x3E; // B C D E F (same as U)
    case 'W': case 'w': return 0x7E; // A B C D E F (same as 0)
    case 'X': case 'x': return 0x37; // B C E F G (same as H)
    case 'Y': case 'y': return 0x3B; // B C D F G
    case 'Z': case 'z': return 0x6D; // A B D E G (same as 2)
    case '-': return 0x01; // G -> bit0 = 0x01 (middle segment)
    case '_': return 0x08; // D
    case '=': return 0x09; // D G
    case ' ': return 0x00; // Blank
    case '.': return 0x80; // DP only
    default: return 0x00; // Blank for unknown characters
  }
}

inline bool isCodeBCompatible(char value) {
  // MAX7219 Code-B decode mode supports: 0-9, -, E, H, L, P, blank
  return (value >= '0' && value <= '9') || 
         value == '-' || 
         value == 'E' || value == 'H' || value == 'L' || value == 'P' || 
         value == ' ';
}

#endif // GLYPHS_H
