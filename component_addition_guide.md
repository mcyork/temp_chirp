# Component Addition Guide: Column Temperature Monitor
**For PCB Layout Tool Schematic Entry**

This guide provides step-by-step instructions for adding all components to the schematic, organized by functional sections.

---

## Section A: Power Subsystem

### Step 1: USB Connector
- **Add Component:** J1
- **Type:** USB Type-C Receptacle
- **Part Number:** LCSC Code `C168688`
- **Package:** 16-Pin, Top-Mount SMD
- **Section:** Power Subsystem

### Step 2: USB CC Resistors
- **Add Component:** R1, R2
- **Type:** Resistor, 5.1kΩ, 1%
- **Power Rating:** 50mW minimum (standard 0402 is 50-63mW, 1/16W)
- **Package:** 0402
- **Application:** USB CC Lines (CC1 and CC2 to GND)
- **Section:** Power Subsystem

### Step 3: LDO Regulator
- **Add Component:** U2
- **Type:** LDO Regulator, 3.3V, 500mA
- **Part Number:** Microsone ME6211C33M5G-N
- **LCSC Code:** `C236681`
- **Package:** SOT-23-5
- **Section:** Power Subsystem

### Step 4: Input Capacitor (LDO)
- **Add Component:** C1
- **Type:** Capacitor, 10uF, 10%
- **Voltage Rating:** 16V or 25V (sees 5V, use 16V minimum)
- **Package:** 0805/0603
- **Application:** LDO Input (VIN to +5V_USB)
- **Section:** Power Subsystem

### Step 5: Output Capacitors (LDO)
- **Add Component:** C2, C3
- **Type:** Capacitor, 22uF (C2) and 0.1uF (C3), 10%
- **Voltage Rating:** 16V or 25V (sees 3.3V, use 16V minimum)
- **Package:** 22uF: 0805/0603, 0.1uF: 0402
- **Application:** LDO Output (VOUT to +3V3)
- **Section:** Power Subsystem

---

## Section B: Microcontroller

### Step 6: ESP32 Module
- **Add Component:** U1
- **Type:** MCU Module, WiFi/BLE, 8MB Flash
- **Part Number:** Espressif ESP32-S3-MINI-1-N8
- **LCSC Code:** `C5295790`
- **Package:** Module
- **Section:** Microcontroller

### Step 7: Reset Circuit Components
- **Add Component:** R3, SW1
- **Type:** 
  - R3: Resistor, 10kΩ, 1% (0402) - Pull-up for EN pin
    - **Power Rating:** 50mW minimum (standard 0402 is 50-63mW, 1/16W)
  - SW1: Tactile Switch, 3.9x3.0mm SMD (LCSC `C720477`) - Reset button
- **Application:** EN pin pull-up and reset switch
- **Section:** Microcontroller

### Step 8: Boot Circuit Components
- **Add Component:** SW2
- **Type:** Tactile Switch, 3.9x3.0mm SMD
- **LCSC Code:** `C720477`
- **Application:** Boot mode switch (IO0 to GND)
- **Section:** Microcontroller

### Step 9: Microcontroller Decoupling Capacitors
- **Add Component:** C4, C5, C6
- **Type:** Capacitor, 0.1uF, 10%
- **Voltage Rating:** 16V+ (sees 3.3V, per spec requirement)
- **Package:** 0402
- **Application:** MCU decoupling (VCC to GND)
- **Section:** Microcontroller

### Step 10: Emergency Header
- **Add Component:** H1
- **Type:** Pin Header, 1x4
- **Package:** 2.54mm Pitch
- **Application:** Emergency UART Port
- **Pin Connections:**
  - Pin 1 → +3V3 net (for external device power/reference)
  - Pin 2 → GND net (common ground reference)
  - Pin 3 → UART_TX net (ESP32 IO43)
  - Pin 4 → UART_RX net (ESP32 IO44)
- **Section:** Microcontroller

---

## Section C: RTD Sensor

### Step 11: RTD Converter IC
- **Add Component:** U3
- **Type:** RTD-to-Digital Converter
- **Part Number:** Maxim MAX31865ATP+
- **LCSC Code:** `C118474`
- **Package:** TQFN-20
- **Section:** RTD Sensor

### Step 12: RTD Reference Resistor (CRITICAL)
- **Add Component:** R4
- **Type:** Resistor, 430Ω, **0.1% tolerance** (CRITICAL)
- **Part Number:** KOA RN73H1JTTD4300B25
- **LCSC Code:** `C186177`
- **Power Rating:** 100mW (standard 0603 rating, actual dissipation <2mW)
- **Package:** 0603 (Metal Film, ±25ppm/°C, 100mW)
- **Application:** RTD Reference (between BIAS and FORCE+)
- **Section:** RTD Sensor

### Step 13: RTD Decoupling Capacitor
- **Add Component:** C7
- **Type:** Capacitor, 0.1uF, 10%
- **Voltage Rating:** 16V+ (sees 3.3V, per spec requirement)
- **Package:** 0402
- **Application:** MAX31865 decoupling
- **Section:** RTD Sensor

### Step 14: Screw Terminal
- **Add Component:** J2
- **Type:** Screw Terminal, 3-Position
- **Part Number:** ZX-DG28L-2.54-3P
- **LCSC Code:** `C49023012`
- **Package:** Through-Hole, 2.54mm Pitch (CONN-TH_3P-P2.54_ZX-DG28L)
- **Application:** RTD sensor connections (FORCE+, RTDIN+, RTDIN-)
- **Section:** RTD Sensor

---

## Section D: Display

### Step 15: Display Driver IC
- **Add Component:** U4
- **Type:** LED Display Driver
- **Part Number:** Maxim MAX7219CWG+T
- **LCSC Code:** `C9964`
- **Package:** SOIC-24
- **Section:** Display

### Step 16: Display Driver Current Set Resistor
- **Add Component:** R5
- **Type:** Resistor, 10kΩ, 1%
- **Power Rating:** 50mW minimum (standard 0402 is 50-63mW, 1/16W)
- **Package:** 0402
- **Application:** MAX7219 ISET pin to 5V
- **Section:** Display

### Step 17: Display Driver Decoupling Capacitor
- **Add Component:** C8
- **Type:** Capacitor, 0.1uF, 10%
- **Voltage Rating:** 16V+ (sees 5V, per spec requirement)
- **Package:** 0402
- **Application:** MAX7219 decoupling
- **Section:** Display

### Step 18: 7-Segment Display
- **Add Component:** DISP1
- **Type:** 7-Segment Display, 4-Digit
- **Part Number:** FJ5461AH
- **LCSC Code:** `C54396`
- **Package:** Through-Hole, Common Cathode, 0.56" Orange-Red (LED-SEG-TH_12P-L50.3-W19.0-P2.54-S15.24-BL)
- **Application:** Temperature display
- **Section:** Display

---

## Section E: User Interface

### Step 19: User Interface Buttons
- **Add Component:** SW3, SW4, SW5
- **Type:** Tactile Switch, 6x6mm SMD
- **LCSC Code:** `C318884`
- **Application:** 
  - SW3: Mode button (to MCU IO5)
  - SW4: Up button (to MCU IO6)
  - SW5: Down button (to MCU IO7)
- **Section:** User Interface

### Step 20: Buzzer Circuit MOSFET
- **Add Component:** Q1
- **Type:** MOSFET, N-Channel, 60V, 380mA
- **Part Number:** LRC L2N7002SLLT1G
- **LCSC Code:** `C22446827`
- **Package:** SOT-23
- **Application:** Buzzer drive circuit
- **Section:** User Interface

### Step 21: Buzzer Gate Resistor
- **Add Component:** R6
- **Type:** Resistor, 1kΩ, 1%
- **Power Rating:** 50mW minimum (standard 0402 is 50-63mW, 1/16W)
- **Package:** 0402
- **Application:** MOSFET gate resistor (MCU IO4 to gate)
- **Section:** User Interface

### Step 22: Buzzer
- **Add Component:** BZ1
- **Type:** Buzzer (Passive)
- **Part Number:** MLT-8530
- **LCSC Code:** `C94599`
- **Package:** SMD, 2.5-4.5V, 80dB, Passive Electromagnetic
- **Application:** Audio feedback (driven by MOSFET Q1)
- **Section:** User Interface

---

## Summary Checklist

**Power Subsystem:** J1, R1, R2, U2, C1, C2, C3  
**Microcontroller:** U1, R3, SW1, SW2, C4, C5, C6, H1  
**RTD Sensor:** U3, R4, C7, J2  
**Display:** U4, R5, C8, DISP1  
**User Interface:** SW3, SW4, SW5, Q1, R6, BZ1  

**Total Components:** 22 components + 11 passive components = 33 total parts

---

## Notes for Designers

1. All passive components use 0402 package unless otherwise specified (0805/0603 for larger capacitors).
2. The 430Ω resistor (R4) **MUST** be 0.1% tolerance - this is critical for RTD accuracy.
3. USB CC resistors (R1, R2) are 5.1kΩ, all other resistors are as specified.
4. Decoupling capacitors (0.1uF) are distributed across multiple sections.
5. Reference designators follow standard conventions (U=IC, R=Resistor, C=Capacitor, Q=Transistor, J=Connector, SW=Switch, etc.).
6. **Resistor Power Ratings:** 
   - 0402 package: Use 50mW minimum (standard is 50-63mW, 1/16W)
   - 0603 package: Use 100mW minimum (standard is 100mW, 1/10W)
   - All resistors in this design have very low power dissipation (<2mW), so standard ratings are sufficient.

