# Schematic Diagrams: Column Temperature Monitor
**Text-Based Schematic Representation**

This document provides visual text-based diagrams showing all component connections, including ground connections for capacitors and other components.

---

## A. Power Subsystem

### USB Connector (J1) → LDO Regulator (U2)

**How to read:** USB-C connectors have multiple VBUS and GND pins (for redundancy). All VBUS pins connect to the same +5V_USB net. All GND pins connect to the same GND net.

```
USB-C Connector (J1) Physical Pins:
├─ VBUS Pin A4 ──────┐
├─ VBUS Pin B4 ──────┤
├─ VBUS Pin A9 ──────┤  (All VBUS pins connect together)
├─ VBUS Pin B9 ──────┘
│                     │
│                     └─── +5V_USB Net
│
├─ GND Pin A1 ───────┐
├─ GND Pin B1 ───────┤
├─ GND Pin A12 ──────┤  (All GND pins connect together)
├─ GND Pin B12 ──────┘
│                     │
│                     └─── GND Net
│
├─ CC1 Pin ────[R1: 5.1kΩ]─── GND
│
├─ CC2 Pin ────[R2: 5.1kΩ]─── GND
│
├─ DN1 Pin ──────────────────── IO19 (USB D-)
├─ DN2 Pin ──────────────────── IO19 (USB D-)
│
├─ DP1 Pin ──────────────────── IO20 (USB D+)
└─ DP2 Pin ──────────────────── IO20 (USB D+)

+5V_USB Net
│
├─── LDO Pin 1 (VIN) ────[C1: 10uF]─── GND
│
└─── LDO Pin 3 (EN)

GND Net ──── LDO Pin 2 (GND)

LDO Regulator (U2 - ME6211C33M5G-N)
Pin 5 (VOUT) ──── +3V3 Net
│
├───[C2: 22uF]─── GND
│
└───[C3: 0.1uF]─── GND
```

**Connection breakdown:**
- **VBUS pins (A4, B4, A9, B9):** USB-C has 4 VBUS pins for redundancy/power handling
  - *All 4 pins are internally connected* on the connector - they're the same electrical point
  - All connect to **+5V_USB net** (one connection point, not 4 separate wires)
- **GND pins (A1, B1, A12, B12):** USB-C has 4 GND pins for redundancy
  - *All 4 pins are internally connected* - they're the same electrical point
  - All connect to **GND net** (common ground plane)
- **CC1/CC2 resistors:** USB Type-C configuration pins - need 5.1kΩ resistors to GND for proper USB-C detection
- **USB Data lines:** 
  - DN1 and DN2 both connect to ESP32 IO19 (USB D-) - redundant pins
  - DP1 and DP2 both connect to ESP32 IO20 (USB D+) - redundant pins
- **+5V_USB net:** Single net that all VBUS pins connect to
  - Powers LDO Pin 1 (VIN) - input voltage
  - Powers LDO Pin 3 (EN) - enable pin (keeps LDO active)
  - Connects to C1 capacitor (input filtering)
- **C1 (10uF):** Input capacitor - one terminal to +5V_USB net, other terminal to GND net
- **LDO Pin 1 (VIN):** Receives 5V from +5V_USB net
- **LDO Pin 2 (GND):** Connects to GND net
- **LDO Pin 3 (EN):** Enable pin - connected to +5V_USB (always enabled when USB plugged in)
- **LDO Pin 5 (VOUT):** Outputs 3.3V → creates +3V3 net
- **C2 (22uF):** Large output capacitor - one terminal to +3V3 net, other to GND net (bulk storage)
- **C3 (0.1uF):** Small output capacitor - one terminal to +3V3 net, other to GND net (high-frequency filtering)

---

## B. Microcontroller (ESP32-S3-MINI-1)

### Power & Control

**How to read:** Shows ESP32 pins and how they connect to power, ground, and control components.

```
ESP32-S3-MINI-1 (U1) Physical Pins:
├─ VCC Pin ────────────────────────┐
│                                   │
├─ GND Pin ────────────────────────┤
│                                   │
├─ EN Pin ──────────────────────────┤
│                                   │
├─ IO0 Pin ─────────────────────────┤
│                                   │
└─ (Internal VCC connection point) ─┘

+3V3 Net ──── VCC Pin
│
└───[C4: 0.1uF]─── GND Net  (Decoupling cap)
└───[C5: 0.1uF]─── GND Net  (Decoupling cap)
└───[C6: 0.1uF]─── GND Net  (Decoupling cap)

GND Net ──── GND Pin

+3V3 Net ────[R3: 10kΩ]─── EN Pin ────[SW1]─── GND Net
             (Pull-up)                  (Reset Button)

IO0 Pin ────[SW2]─── GND Net
            (Boot Button)
```

**Connection breakdown:**
- **Line 1:** +3V3 power net connects to ESP32 VCC pin (powers the chip)
- **Line 2:** GND net connects to ESP32 GND pin (ground reference)
- **Line 3:** +3V3 → Resistor R3 (10kΩ pull-up) → ESP32 EN pin → Switch SW1 → GND
  - *When SW1 is NOT pressed:* EN pin sees 3.3V (normal operation)
  - *When SW1 IS pressed:* EN pin connects to GND (chip resets)
- **Line 4:** ESP32 IO0 pin → Switch SW2 → GND
  - *When SW2 is pressed:* IO0 goes to GND (enters boot mode)
- **Lines 5-7:** Three decoupling capacitors (C4, C5, C6) connect between VCC and GND
  - These filter noise and provide local power storage

### Data Interfaces

**How to read:** Shows ESP32 communication pins and which nets/devices they connect to.

```
ESP32-S3-MINI-1 (U1) Physical Pins:
├─ IO16 Pin ────────────────────────┐
├─ IO17 Pin ────────────────────────┤
├─ IO18 Pin ────────────────────────┤
├─ IO19 Pin ────────────────────────┤
├─ IO20 Pin ────────────────────────┤
├─ IO43 Pin ────────────────────────┤
└─ IO44 Pin ────────────────────────┘

IO16 Pin ──── SCK Net (SPI Clock)
              │
              ├─── MAX31865 SCK Pin (U3)
              └─── MAX7219 CLK Pin (U4)

IO17 Pin ──── MOSI Net (SPI Master Out)
              │
              ├─── MAX31865 MOSI Pin (U3)
              └─── MAX7219 DIN Pin (U4)

IO18 Pin ──── MISO Net (SPI Master In)
              │
              └─── MAX31865 MISO Pin (U3)

IO19 Pin ──── D- Net (USB Data Negative)
              │
              └─── USB Connector DN1/DN2 Pins (J1)

IO20 Pin ──── D+ Net (USB Data Positive)
              │
              └─── USB Connector DP1/DP2 Pins (J1)

IO43 Pin ──── UART_TX Net
              │
              └─── Emergency Header Pin 3 (H1)

IO44 Pin ──── UART_RX Net
              │
              └─── Emergency Header Pin 4 (H1)

+3V3 Net ──── Emergency Header Pin 1 (H1)

GND Net ──── Emergency Header Pin 2 (H1)
```

**Connection breakdown:**
- **IO16 (SCK):** SPI Clock - shared between RTD (U3) and Display (U4) - synchronizes data transfer
- **IO17 (MOSI):** Master Out Slave In - shared between RTD and Display - ESP32 sends data to these devices
- **IO18 (MISO):** Master In Slave Out - only RTD uses this - RTD sends data back to ESP32
- **IO19 (D-):** USB Data Negative - connects to USB connector DN1/DN2 pins - for USB communication
- **IO20 (D+):** USB Data Positive - connects to USB connector DP1/DP2 pins - for USB communication
- **IO43 (UART_TX):** UART Transmit - connects to emergency header pin 3 - for debugging/emergency access
- **IO44 (UART_RX):** UART Receive - connects to emergency header pin 4 - for debugging/emergency access
- **Emergency Header (H1):** 4-pin header for external UART connection
  - Pin 1: +3V3 (provides power/reference for external devices)
  - Pin 2: GND (common ground reference)
  - Pin 3: UART_TX (transmit from ESP32)
  - Pin 4: UART_RX (receive to ESP32)

---

## C. RTD Sensor (MAX31865)

### SPI Connections

**How to read:** Shows MAX31865 pins and how they connect to ESP32 SPI bus and power nets.

```
MAX31865 (U3) Physical Pins:
├─ SCLK Pin ────────────────────────┐  (Serial Clock - labeled SCLK on chip)
├─ SDI Pin ─────────────────────────┤  (Serial Data In - labeled SDI on chip, connects to MOSI)
├─ SDO Pin ─────────────────────────┤  (Serial Data Out - labeled SDO on chip, connects to MISO)
├─ CS Pin ──────────────────────────┤  (Chip Select)
├─ VDD Pin ────────────────────────┤
├─ GND Pin ────────────────────────┤
└─ (Other pins not shown) ──────────┘

SCK Net ──── SCLK Pin (SPI Clock)
MOSI Net ──── SDI Pin (Data In - MOSI from ESP32 perspective)
MISO Net ──── SDO Pin (Data Out - MISO from ESP32 perspective)

ESP32 IO10 Pin ──── CS Pin (Chip Select)

+3V3 Net ──── VDD Pin ────[C7: 0.1uF]─── GND Net
                              (Decoupling cap)

GND Net ──── GND Pin
```

**Connection breakdown:**
- **SCK (Clock):** ESP32 IO16 → MAX31865 SCLK - ESP32 generates clock signal to synchronize data transfer
  - *Note: MAX31865 pin is labeled "SCLK" (not SCK)*
- **MOSI (Master Out):** ESP32 IO17 → MAX31865 SDI - ESP32 sends commands/data to MAX31865
  - *Note: MAX31865 pin is labeled "SDI" (Serial Data In), which is MOSI from ESP32 perspective*
- **MISO (Master In):** ESP32 IO18 → MAX31865 SDO - MAX31865 sends temperature data back to ESP32
  - *Note: MAX31865 pin is labeled "SDO" (Serial Data Out), which is MISO from ESP32 perspective*
- **CS (Chip Select):** ESP32 IO10 → MAX31865 CS - When LOW, MAX31865 is active; when HIGH, it's deselected
- **VDD:** +3V3 net → MAX31865 VDD - Powers the RTD sensor chip (3.3V)
- **GND:** Common GND → MAX31865 GND - Ground reference
- **C7 (0.1uF):** Decoupling capacitor - one terminal to VDD, other to GND - filters power supply noise

### RTD Analog Path (3-Wire)

**How to read:** Shows screw terminal pins and how they connect to MAX31865 RTD measurement pins.

```
Screw Terminal (J2) Physical Pins:
├─ Pin 1 ────────────────────────┐
├─ Pin 2 ────────────────────────┤
└─ Pin 3 ────────────────────────┘

MAX31865 (U3) Physical Pins:
├─ FORCE+ Pin ──────────────────┐
├─ RTDIN+ Pin (FORCE2) ───────────┤
├─ RTDIN- Pin (FORCE-) ───────────┤
└─ BIAS Pin ──────────────────────┘

Screw Terminal Pin 1 ──── FORCE+ Pin ────[R4: 430Ω 0.1%]─── BIAS Pin
                                         (Critical Reference Resistor)

Screw Terminal Pin 2 ──── RTDIN+ Pin (FORCE2)

Screw Terminal Pin 3 ──── RTDIN- Pin (FORCE-)
```

**Connection breakdown:**
- **Screw Terminal Pin 1:** Connects to RTD probe wire 1 → MAX31865 FORCE+ pin
- **R4 (430Ω, 0.1%):** Critical precision resistor - connects between FORCE+ and BIAS pins
  - *Why 0.1% tolerance?* This resistor is the reference for RTD measurement - any error here causes temperature reading errors
  - *Why 430Ω?* This matches the RTD reference resistance value needed by MAX31865
- **Screw Terminal Pin 2:** Connects to RTD probe wire 2 → MAX31865 RTDIN+ (also called FORCE2)
- **Screw Terminal Pin 3:** Connects to RTD probe wire 3 → MAX31865 RTDIN- (also called FORCE-)
- **BIAS pin:** Provides bias current through R4 to the RTD probe
- **3-Wire RTD:** Uses 3 wires to compensate for wire resistance - improves accuracy over long cable runs

---

## D. Display (MAX7219 & 7-Segment)

### SPI Connections

**How to read:** Shows MAX7219 pins and how they connect to ESP32 SPI bus and power nets.

```
MAX7219 (U4) Physical Pins:
├─ DIN Pin (Pin 1) ────────────────────────┐
├─ CLK Pin (Pin 13) ───────────────────────┤
├─ LOAD Pin (Pin 12) ───────────────────────┤
├─ VCC Pin (Pin 19 - V+) ────────────────────────┤
├─ GND Pin (Pin 4, 9) ────────────────────────┤
├─ ISET Pin (Pin 18) ───────────────────────┘
└─ DOUT Pin (Pin 24) ──────────────────────── (Not used - no MISO needed)

MOSI Net ──── DIN Pin (Data In)
SCK Net ──── CLK Pin (Clock)

ESP32 IO11 Pin ──── LOAD Pin (Chip Select)

+5V_USB Net ──── VCC Pin ────[C8: 0.1uF]─── GND Net
                              (Decoupling cap)

GND Net ──── GND Pin

+5V_USB Net ────[R5: 10kΩ]─── ISET Pin (Current Set)
```

**Connection breakdown:**
- **DIN (Data In):** ESP32 IO17 → MAX7219 DIN - ESP32 sends display data to MAX7219
- **CLK (Clock):** ESP32 IO16 → MAX7219 CLK - Clock signal synchronizes data transfer (shared with RTD)
- **LOAD (Chip Select):** ESP32 IO11 → MAX7219 LOAD - When LOW, MAX7219 accepts data; when HIGH, it latches data
- **VCC:** +5V_USB net → MAX7219 VCC - Display driver runs on 5V for better LED drive strength
- **GND:** Common GND → MAX7219 GND - Ground reference
- **C8 (0.1uF):** Decoupling capacitor - one terminal to VCC, other to GND - filters power supply noise
- **ISET:** +5V_USB → Resistor R5 (10kΩ) → MAX7219 ISET - Sets LED current/brightness
  - *Why 5V?* Display needs more voltage than 3.3V to drive LEDs brightly

### Display Connections

**How to read:** Shows MAX7219 display output pins and how they connect to the 7-segment display.

```
MAX7219 (U4) Physical Pins:
├─ DIG0 Pin ───────────────────────┐
├─ DIG1 Pin ────────────────────────┤
├─ DIG2 Pin ────────────────────────┤
├─ DIG3 Pin ────────────────────────┤
├─ SegA Pin ────────────────────────┤
├─ SegB Pin ────────────────────────┤
├─ SegC Pin ────────────────────────┤
├─ SegD Pin ────────────────────────┤
├─ SegE Pin ────────────────────────┤
├─ SegF Pin ────────────────────────┤
├─ SegG Pin ────────────────────────┤
└─ SegDP Pin ───────────────────────┘

7-Segment Display (DISP1) Physical Pins:
├─ Common Cathode 0 ─────────────────┐
├─ Common Cathode 1 ─────────────────┤
├─ Common Cathode 2 ─────────────────┤
├─ Common Cathode 3 ─────────────────┤
├─ Segment A ────────────────────────┤
├─ Segment B ────────────────────────┤
├─ Segment C ────────────────────────┤
├─ Segment D ────────────────────────┤
├─ Segment E ────────────────────────┤
├─ Segment F ────────────────────────┤
├─ Segment G ────────────────────────┤
└─ Decimal Point ─────────────────────┘

DIG0 Pin ──── Common Cathode 0
DIG1 Pin ──── Common Cathode 1
DIG2 Pin ──── Common Cathode 2
DIG3 Pin ──── Common Cathode 3

SegA Pin ──── Segment A
SegB Pin ──── Segment B
SegC Pin ──── Segment C
SegD Pin ──── Segment D
SegE Pin ──── Segment E
SegF Pin ──── Segment F
SegG Pin ──── Segment G
SegDP Pin ──── Decimal Point
```

**Connection breakdown:**
- **DIG0-DIG3:** Digit select pins - each connects to one digit's common cathode
  - MAX7219 rapidly switches between digits (multiplexing) to display all 4 digits
  - Only one digit is active at a time, but it switches so fast you see all digits
- **SegA-SegG:** Segment control pins - control which segments (A-G) light up to form numbers
  - SegA = top horizontal, SegB = top-right vertical, SegC = bottom-right vertical, etc.
  - SegG = middle horizontal (for numbers like 8, 0, 6)
- **SegDP:** Decimal point segment - controls the decimal point between digits
- **How it works:** MAX7219 turns on one digit (DIG pin LOW), lights up segments (Seg pins HIGH), then switches to next digit - repeats rapidly

---

## E. User Interface

### Buttons

**How to read:** Shows button switches and how they connect ESP32 pins to ground.

```
ESP32-S3-MINI-1 (U1) Physical Pins:
├─ IO5 Pin ────────────────────────┐
├─ IO6 Pin ───────────────────────┤
└─ IO7 Pin ────────────────────────┘

Tactile Switches:
├─ SW3 (Mode Button) ───────────────┐
├─ SW4 (Up Button) ────────────────┤
└─ SW5 (Down Button) ──────────────┘

IO5 Pin ────[SW3]─── GND Net (Mode Button)
            (When pressed: IO5 connects to GND)

IO6 Pin ────[SW4]─── GND Net (Up Button)
            (When pressed: IO6 connects to GND)

IO7 Pin ────[SW5]─── GND Net (Down Button)
            (When pressed: IO7 connects to GND)
```

**Connection breakdown:**
- **IO5 → SW3 (Mode):** When SW3 pressed, IO5 connects to GND (reads LOW) - cycles through modes (RUN → SET THRESHOLD → SET STEP)
- **IO6 → SW4 (Up):** When SW4 pressed, IO6 connects to GND (reads LOW) - increases values in SET modes
- **IO7 → SW5 (Down):** When SW5 pressed, IO7 connects to GND (reads LOW) - decreases values in SET modes
- **How it works:** ESP32 configures these pins with INPUT_PULLUP - internal resistor pulls pin HIGH when button not pressed
  - *Button NOT pressed:* Pin reads HIGH (3.3V) via internal pull-up
  - *Button pressed:* Pin reads LOW (0V) because switch connects pin to GND

### Buzzer Circuit

**How to read:** Shows MOSFET switch and buzzer connections. MOSFET controls buzzer power from 5V supply.

```
ESP32-S3-MINI-1 (U1) Physical Pin:
└─ IO4 Pin ────────────────────────┐

MOSFET (Q1 - L2N7002SLLT1G) Physical Pins:
├─ Gate Pin ────────────────────────┤
├─ Source Pin ──────────────────────┤
└─ Drain Pin ───────────────────────┤

Buzzer (BZ1) Physical Terminals:
├─ Positive Terminal (+) ────────────┤
└─ Negative Terminal (-) ────────────┘

IO4 Pin ────[R6: 1kΩ]─── Gate Pin
            (Gate resistor)

Source Pin ──── GND Net

+5V_USB Net ──── Buzzer(+) Terminal

Drain Pin ──── Buzzer(-) Terminal
```

**Connection breakdown:**
- **IO4:** ESP32 output pin - controls buzzer on/off
- **R6 (1kΩ):** Gate resistor - limits current to MOSFET gate, protects ESP32 pin
- **MOSFET Gate:** Control input - when HIGH (3.3V), MOSFET turns ON; when LOW (0V), MOSFET turns OFF
- **MOSFET Source:** Connects to GND - this is the "low side" of the switch
- **MOSFET Drain:** Connects to buzzer negative terminal - when MOSFET ON, this connects buzzer(-) to GND
- **Buzzer positive:** Connects to +5V_USB - always has 5V power
- **Buzzer negative:** Connects to MOSFET Drain - completes circuit when MOSFET ON
- **How it works:** 
  - *ESP32 sets IO4 HIGH:* MOSFET turns ON → Drain connects to Source (GND) → Buzzer(-) goes to GND → Current flows: +5V → Buzzer → MOSFET → GND → Buzzer sounds
  - *ESP32 sets IO4 LOW:* MOSFET turns OFF → No current flows → Buzzer silent
  - ESP32 uses PWM (pulse width modulation) on IO4 to create different tones/chirps

---

## Complete Power Distribution

**How to read:** Shows how +5V_USB net distributes power to all components that need 5V, and how GND net connects everything together.

```
+5V_USB Net Connections:
├─ USB Connector VBUS Pins (J1) ────┐
│                                    │
├─ LDO Pin 1 (VIN) ─────────────────┤
│    │                               │
│    └───[C1: 10uF]─── GND Net       │
│                                    │
├─ LDO Pin 3 (EN) ──────────────────┤
│                                    │
├─ MAX7219 VCC Pin (U4) ────────────┤
│    │                               │
│    └───[C8: 0.1uF]─── GND Net     │
│                                    │
├─ MAX7219 ISET Pin ────[R5: 10kΩ] ─┤
│                                    │
└─ Buzzer(+) Terminal (BZ1) ─────────┘

GND Net Connections (Common Ground Plane):
├─ USB Connector GND Pins (J1) ─────┐
│                                    │
├─ LDO Pin 2 (GND) ─────────────────┤
│                                    │
├─ ESP32 GND Pin (U1) ──────────────┤
│                                    │
├─ MAX31865 GND Pin (U3) ───────────┤
│                                    │
├─ MAX7219 GND Pin (U4) ────────────┤
│                                    │
├─ MOSFET Source Pin (Q1) ──────────┤
│                                    │
├─ All Capacitor Ground Terminals ───┤
│   (C1, C2, C3, C4, C5, C6, C7, C8)│
│                                    │
├─ All Resistor Ground Connections ─┤
│   (R1, R2, R3, R5)                │
│                                    │
└─ All Switch Ground Connections ────┘
   (SW1, SW2, SW3, SW4, SW5)
```

---

## Complete 3.3V Distribution

**How to read:** Shows how +3V3 net distributes power from LDO output to all components that need 3.3V.

```
+3V3 Net Connections:
├─ LDO Pin 5 (VOUT) ─────────────────┐
│    │                                 │
│    ├───[C2: 22uF]─── GND Net        │
│    │                                 │
│    └───[C3: 0.1uF]─── GND Net      │
│                                      │
├─ ESP32 VCC Pin (U1) ────────────────┤
│    │                                 │
│    ├───[C4: 0.1uF]─── GND Net       │
│    ├───[C5: 0.1uF]─── GND Net       │
│    └───[C6: 0.1uF]─── GND Net       │
│                                      │
├─ MAX31865 VDD Pin (U3) ─────────────┤
│    │                                 │
│    └───[C7: 0.1uF]─── GND Net       │
│                                      │
└─ EN Pull-up ────[R3: 10kΩ]─── ESP32 EN Pin (U1)
```

---

## Key Points

1. **All capacitors have two terminals:**
   - One terminal connects to a signal/power net
   - The other terminal **always** connects to **GND**

2. **Power flow:**
   - USB VBUS (5V) → LDO Input → LDO Output (3.3V) → System Components
   - Display and Buzzer run directly from 5V for better drive strength

3. **Ground is common:**
   - All GND connections tie together to a common ground plane
   - This includes: USB GND, capacitor grounds, resistor grounds, switch grounds

4. **SPI bus is shared:**
   - SCK and MOSI are shared between RTD (U3) and Display (U4)
   - MISO is only used by RTD (U3)
   - Each device has its own Chip Select (CS) line

