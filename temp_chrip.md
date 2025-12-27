

**How to turn this into a PDF:**
1.  Copy the text between the horizontal lines below.
2.  Paste it into Microsoft Word or Google Docs.
3.  **File > Download > PDF Document (.pdf)**.

***

# Production Design Specification: Column Temperature Monitor
**Revision:** 1.0 (Factory Optimized)
**Date:** December 24, 2025
**Status:** Ready for Schematic Capture

---

## 1. Executive Summary
This document defines the electrical architecture for the Column Temperature Monitor. This design is optimized for **Automated Assembly (PCBA)** using surface-mount components, an ESP32-S3-MINI module, and a simplified factory-friendly Bill of Materials.

---

## 2. Optimized Bill of Materials (BOM)
*All components selected for availability at LCSC (EasyEDA Native).*

### Active Components
| Ref Des | Qty | Description | Manufacturer / Part | Package | LCSC Code |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **U1** | 1 | MCU, WiFi/BLE, 8MB Flash | Espressif **ESP32-S3-MINI-1-N8** | Module | `C5295790` |
| **U2** | 1 | LDO Reg, 3.3V, 500mA | Microsone **ME6211C33M5G-N** | SOT-23-5 | `C236681` |
| **U3** | 1 | RTD-to-Digital Converter | Maxim **MAX31865ATP+** | TQFN-20 | `C118474` |
| **U4** | 1 | LED Display Driver | Maxim **MAX7219CWG+T** | SOIC-24 | `C9964` |
| **Q1** | 1 | MOSFET, N-Ch, 60V | LRC **L2N7002SLLT1G** | SOT-23 | `C22446827` |

### Connectors & Electromechanical
| Ref Des | Qty | Description | Details | LCSC Code |
| :--- | :--- | :--- | :--- | :--- |
| **J1** | 1 | USB Type-C Receptacle | 16-Pin, Top-Mount SMD | `C168688` |
| **J2** | 1 | Screw Terminal, 3-Pos | ZX-DG28L-2.54-3P, 2.54mm Pitch, TH | `C49023012` |
| **DISP1**| 1 | 7-Segment Display, 4-Digit | FJ5461AH, Common Cathode, 0.56" Orange-Red, TH | `C54396` |
| **SW1,2**| 2 | Tactile Switch (Reset/Boot) | 3.9x3.0mm SMD | `C720477` |
| **SW3-5**| 3 | Tactile Switch (User UI) | 6x6mm SMD | `C318884` |
| **BZ1** | 1 | Buzzer (Passive) | MLT-8530, 2.5-4.5V, 80dB, SMD | `C94599` |
| **H1** | 1 | Pin Header, 1x4 | 2.54mm Pitch (Emergency Port) | *Generic* |

### Passive Components (0402 Preferred)
| Value | Tolerance | Application | Qty |
| :--- | :--- | :--- | :--- |
| **430Ω** | **0.1%** | **RTD Reference (CRITICAL)** - KOA RN73H1JTTD4300B25, 0603, LCSC `C186177` | 1 |
| **5.1kΩ** | 1% | USB CC Lines | 2 |
| **10kΩ** | 1% | Pull-ups / ISET | 5 |
| **1kΩ** | 1% | MOSFET Gate | 1 |
| **0.1uF** | 10% | Decoupling (16V+) | 6 |
| **10uF** | 10% | Bulk Cap (0805/0603) | 2 |
| **22uF** | 10% | Bulk Cap (0805/0603) | 2 |

---

## 3. Schematic Nets & Connections

### Net List (All Unique Nets)

`+5V_USB`, `+3V3`, `GND`, `MOSI`, `MISO`, `SCK`, `CS_RTD`, `CS_DISP`, `D-`, `D+`, `UART_TX`, `UART_RX`, `EN`, `IO0`, `IO4`, `IO5`, `IO6`, `IO7`, `BIAS`, `FORCE+`, `RTDIN+`, `RTDIN-`, `ISET`, `MOSFET_GATE`, `MOSFET_DRAIN`, `BUZZER_POS`, `BUZZER_NEG`, `DIG0-3`, `SegA-G,DP`

**Total Unique Nets: 28**

---

### A. Power Subsystem
**Input:** USB-C (5V) → **Output:** System Rail (3.3V)

*   **USB Connector (J1):**
    *   `VBUS` (Pins A4, B4, A9, B9) → Net: `+5V_USB`.
    *   `GND` (Pins A1, B1, A12, B12) → Net: `GND`.
    *   `CC1`, `CC2` → 5.1kΩ Resistors → `GND`.
    *   **USB Data Pins:** `DN1`, `DN2` (D-), `DP1`, `DP2` (D+) → Differential Pair to MCU.
        *   `DN1`, `DN2` → MCU `IO 19` (USB D-)
        *   `DP1`, `DP2` → MCU `IO 20` (USB D+) 

*   **LDO Regulator (U2 - ME6211):**
    *   Pin 1 (VIN) → `+5V_USB` (+ **C1** 10uF Cap).
    *   Pin 2 (GND) → `GND`.
    *   Pin 3 (EN) → `+5V_USB` (Enable High).
    *   Pin 5 (VOUT) → `+3V3` (+ **C2** 22uF & **C3** 0.1uF Caps).

### B. Microcontroller (ESP32-S3-MINI-1)
*   **Power:** 3.3V to `VCC`, `GND` to `GND`.
*   **Control:**
    *   `EN`: 10kΩ Pull-up to 3.3V. **SW1 (Reset)** to GND.
    *   `IO0`: **SW2 (Boot)** to GND.
*   **Data Interfaces:**
    *   `IO 16`: SPI `SCK` (Shared with RTD & Display)
    *   `IO 17`: SPI `MOSI` (Shared with RTD & Display)
    *   `IO 18`: SPI `MISO` (RTD only)
    *   `IO 19`: USB `D-`
    *   `IO 20`: USB `D+`
    *   `IO 43`: UART TX (To Emergency Header Pin 3)
    *   `IO 44`: UART RX (To Emergency Header Pin 4)

### C. RTD Sensor (MAX31865 - TQFN)
*   **SPI Bus:** Shared `MOSI` (IO 17), `MISO` (IO 18), `SCK` (IO 16).
    *   ESP32 IO16 → MAX31865 `SCLK` (Serial Clock)
    *   ESP32 IO17 → MAX31865 `SDI` (Serial Data In - MOSI from ESP32)
    *   ESP32 IO18 → MAX31865 `SDO` (Serial Data Out - MISO from ESP32)
*   **Chip Select:** MCU `IO 10` → MAX31865 `CS`.
*   **Analog Path (3-Wire):**
    *   **Bias:** 430Ω (0.1%) Resistor between `BIAS` and `FORCE+`.
    *   **Terminals:** Connect `FORCE+`, `FORCE2`/`RTDIN+`, and `RTDIN-`/`FORCE-` to Screw Terminal J2.

### D. Display (MAX7219 & 7-Seg)
*   **Power:** Connect VCC to `+5V_USB` (Better drive strength).
*   **SPI Bus:** Shared `MOSI` (IO 17), `SCK` (IO 16).
*   **Chip Select:** MCU `IO 11` → MAX7219 `LOAD`.
*   **Current Set:** 10kΩ Resistor from `ISET` to 5V.
*   **LED Mapping:** `DIG0-3` to Common Cathodes; `SegA-G,DP` to Anodes.

### E. User Interface
*   **Buttons:**
    *   Mode → MCU `IO 5`
    *   Up → MCU `IO 6`
    *   Down → MCU `IO 7`
    *   *(All switches connect Pin 2 to GND).*
*   **Buzzer:**
    *   MCU `IO 4` → 1kΩ → MOSFET Gate.
    *   Buzzer(+) → 5V. Buzzer(-) → MOSFET Drain.

---

## 4. Layout & Manufacturing Guidelines

1.  **Placement Strategy ("The Stick"):**
    *   **Top:** Screw Terminal & Analog RTD Section (Keep isolated).
    *   **Center:** Display & User Buttons.
    *   **Bottom:** MCU, USB, and Power Regulation.
2.  **Analog Isolation:** Place the 430Ω reference resistor and C_bypass **immediately** next to the MAX31865 pins. Keep digital traces away from the RTD input lines.
3.  **USB Data:** Route D+ and D- as a differential pair (keep them parallel and equal length).
4.  **Thermal:** The ESP32-S3-MINI has a ground pad underneath. Ensure this is soldered to the ground plane with multiple vias for heat dissipation.
5.  **Test Points:** Add copper test pads on the bottom layer for: `3V3`, `GND`, `RX`, `TX`, `GPIO0`.

---
*End of Specification*