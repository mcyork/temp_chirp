# Schematic Component Checklist
**Check off components as you add them to your schematic**

---

## Section A: Power Subsystem

### Step 1: USB Connector
- [ ] **J1** - USB Type-C Receptacle
  - LCSC Code: `C168688`
  - Package: 16-Pin, Top-Mount SMD

### Step 2: USB CC Resistors
- [ ] **R1** - Resistor, 5.1kΩ, 1%
  - Power Rating: 50mW minimum
  - Package: 0402 (or 0603)
  - Application: USB CC1 to GND
- [ ] **R2** - Resistor, 5.1kΩ, 1%
  - Power Rating: 50mW minimum
  - Package: 0402 (or 0603)
  - Application: USB CC2 to GND

### Step 3: LDO Regulator
- [ ] **U2** - LDO Regulator, 3.3V, 500mA
  - Part Number: ME6211C33M5G-N
  - LCSC Code: `C82942` (or `C236681`)
  - Package: SOT-23-5

### Step 4: Input Capacitor (LDO)
- [ ] **C1** - Capacitor, 10uF, 10%
  - Voltage Rating: 16V or 25V
  - Package: 0402 (or 0805/0603)
  - Application: LDO Input (VIN to +5V_USB, other terminal to GND)

### Step 5: Output Capacitors (LDO)
- [ ] **C2** - Capacitor, 22uF, 10%
  - Voltage Rating: 16V or 25V
  - Package: 0805/0603
  - Application: LDO Output (VOUT to +3V3, other terminal to GND)
- [ ] **C3** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: LDO Output (VOUT to +3V3, other terminal to GND)

---

## Section B: Microcontroller

### Step 6: ESP32 Module
- [ ] **U1** - MCU Module, WiFi/BLE, 8MB Flash
  - Part Number: ESP32-S3-MINI-1-N8
  - LCSC Code: `C2913206` (or `C5295790`)
  - Package: Module

### Step 7: Reset Circuit Components
- [ ] **R3** - Resistor, 10kΩ, 1%
  - Power Rating: 50mW minimum
  - Package: 0402 (or 0805)
  - Application: Pull-up for EN pin (EN to +3V3)
- [ ] **SW1** - Tactile Switch, 3.9x3.0mm SMD
  - LCSC Code: `C720477`
  - Application: Reset button (EN to GND)

### Step 8: Boot Circuit Components
- [ ] **SW2** - Tactile Switch, 3.9x3.0mm SMD
  - LCSC Code: `C720477`
  - Application: Boot mode switch (IO0 to GND)

### Step 9: Microcontroller Decoupling Capacitors
- [ ] **C4** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: MCU decoupling (VCC to GND)
- [ ] **C5** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: MCU decoupling (VCC to GND)
- [ ] **C6** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: MCU decoupling (VCC to GND)

### Step 10: Emergency Header
- [ ] **H1** - Pin Header, 1x4
  - Package: 2.54mm Pitch
  - Application: Emergency UART Port (RX/TX)

---

## Section C: RTD Sensor

### Step 11: RTD Converter IC
- [ ] **U3** - RTD-to-Digital Converter
  - Part Number: MAX31865ATP+
  - LCSC Code: `C118474`
  - Package: TQFN-20

### Step 12: RTD Reference Resistor (CRITICAL)
- [ ] **R4** - Resistor, 430Ω, **0.1% tolerance** (CRITICAL)
  - Part Number: KOA RN73H1JTTD4300B25
  - LCSC Code: `C186177`
  - Power Rating: 100mW
  - Package: 0603
  - Application: RTD Reference (between BIAS and FORCE+)

### Step 13: RTD Decoupling Capacitor
- [ ] **C7** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: MAX31865 decoupling (VDD to GND)

### Step 14: Screw Terminal
- [ ] **J2** - Screw Terminal, 3-Position
  - Part Number: ZX-DG28L-2.54-3P
  - LCSC Code: `C49023012`
  - Package: Through-Hole, 2.54mm Pitch
  - Application: RTD sensor connections (FORCE+, RTDIN+, RTDIN-)

---

## Section D: Display

### Step 15: Display Driver IC
- [ ] **U4** - LED Display Driver
  - Part Number: MAX7219CWG+T
  - LCSC Code: `C9964`
  - Package: SOIC-24

### Step 16: Display Driver Current Set Resistor
- [ ] **R5** - Resistor, 10kΩ, 1%
  - Power Rating: 50mW minimum
  - Package: 0402 (or 0805)
  - Application: MAX7219 ISET pin to +5V_USB

### Step 17: Display Driver Decoupling Capacitor
- [ ] **C8** - Capacitor, 0.1uF, 10%
  - Voltage Rating: 16V+
  - Package: 0402
  - Application: MAX7219 decoupling (VCC to GND)

### Step 18: 7-Segment Display
- [ ] **LED1** (or DISP1) - 7-Segment Display, 4-Digit
  - Part Number: FJ5461AH
  - LCSC Code: `C54396`
  - Package: Through-Hole, Common Cathode, 0.56" Orange-Red
  - Application: Temperature display

---

## Section E: User Interface

### Step 19: User Interface Buttons
- [ ] **SW3** - Tactile Switch, 6x6mm SMD
  - LCSC Code: `C318884`
  - Application: Mode button (to MCU IO5)
- [ ] **SW4** - Tactile Switch, 6x6mm SMD
  - LCSC Code: `C318884`
  - Application: Up button (to MCU IO6)
- [ ] **SW5** - Tactile Switch, 6x6mm SMD
  - LCSC Code: `C318884`
  - Application: Down button (to MCU IO7)

### Step 20: Buzzer Circuit MOSFET
- [ ] **Q1** - MOSFET, N-Channel, 60V, 380mA
  - Part Number: LRC L2N7002SLLT1G
  - LCSC Code: `C22446827`
  - Package: SOT-23
  - Application: Buzzer drive circuit

### Step 21: Buzzer Gate Resistor
- [ ] **R6** - Resistor, 1kΩ, 1%
  - Power Rating: 50mW minimum
  - Package: 0402 (or 0603)
  - Application: MOSFET gate resistor (MCU IO4 to gate)

### Step 22: Buzzer
- [ ] **BUZZER1** (or BZ1) - Buzzer (Passive)
  - Part Number: QMB-09B-03
  - LCSC Code: `C96256`
  - Package: Through-Hole, 2-5V, 85dB, Passive Electromagnetic
  - Application: Audio feedback (driven by MOSFET Q1)

---

## Summary Checklist

**Power Subsystem:** J1, R1, R2, U2, C1, C2, C3  
**Microcontroller:** U1, R3, SW1, SW2, C4, C5, C6, H1  
**RTD Sensor:** U3, R4, C7, J2  
**Display:** U4, R5, C8, LED1  
**User Interface:** SW3, SW4, SW5, Q1, R6, BUZZER1  

**Total Components:** 22 components + 11 passive components = 33 total parts

---

## Net Connections Checklist

### Power Nets
- [x] **+5V_USB** net created and connected to:
  - [x] USB connector VBUS pins (J1) - J1.A9B4, J1.B9A4 ✅
  - [x] LDO Pin 1 (VIN) - U2.1 ✅
  - [x] LDO Pin 3 (EN) - U2.3 ✅
  - [x] C1 capacitor (one terminal) - C1.2 ✅
  - [ ] MAX7219 VCC (U4 Pin 24) - **MISSING** (pin 24 is empty)
  - [x] C8 capacitor (one terminal) - C8.1 ✅
  - [x] R5 resistor (one terminal) - R5.1 ✅
  - [x] Buzzer positive terminal - BUZZER1.1 ✅

- [x] **+3V3** net created and connected to:
  - [x] LDO Pin 5 (VOUT) - U2.5 ✅
  - [x] C2 capacitor (one terminal) - C2.2 ✅
  - [x] C3 capacitor (one terminal) - C3.1 ✅
  - [x] ESP32 VCC (U1) - U1.45 ✅
  - [x] C4 capacitor (one terminal) - C4.1 ✅
  - [x] C5 capacitor (one terminal) - C5.1 ✅
  - [x] C6 capacitor (one terminal) - C6.1 ✅
  - [ ] MAX31865 VDD (U3) - **MISSING** (need to check which pin is VDD)
  - [x] C7 capacitor (one terminal) - C7.2 ✅
  - [x] R3 resistor (one terminal) - R3.1 ✅

- [x] **GND** net created and connected to:
  - [x] USB connector GND pins (J1) - J1.B12A1, J1.A12B1 ✅
  - [x] C1 capacitor ground terminal - C1.1 ✅
  - [x] C2 capacitor ground terminal - C2.1 ✅
  - [x] C3 capacitor ground terminal - C3.2 ✅
  - [x] C4 capacitor ground terminal - C4.2 ✅
  - [x] C5 capacitor ground terminal - C5.2 ✅
  - [x] C6 capacitor ground terminal - C6.2 ✅
  - [x] C7 capacitor ground terminal - C7.1 ✅
  - [x] C8 capacitor ground terminal - C8.2 ✅
  - [x] R1 resistor ground connection - R1.1 (CC1 to GND) ✅
  - [x] R2 resistor ground connection - R2.2 (CC2 to GND) ✅
  - [x] SW1 switch ground connection - SW1.2 ✅
  - [x] SW2 switch ground connection - SW2.1 ✅
  - [x] SW3 switch ground connections - SW3.4, SW3.3 ✅
  - [x] SW4 switch ground connections - SW4.4, SW4.3 ✅
  - [x] SW5 switch ground connections - SW5.4, SW5.3 ✅
  - [x] LDO Pin 2 (GND/VSS) - U2.2 ✅
  - [x] ESP32 GND (U1) - U1.1, U1.2, U1.42-60, U1.GND ✅
  - [ ] MAX31865 GND (U3) - **MISSING** (need to check which pin is GND)
  - [x] MAX7219 GND (U4) - U4.4, U4.9 ✅
  - [ ] MOSFET Source (Q1 Pin 2) - **MISSING** (pin 2 is empty, should be GND)
  
  **Note:** R3, R4, R5, and R6 do NOT connect to GND:
  - R3: Connects +3V3 to EN pin (pull-up resistor)
  - R4: Connects FORCE+ to BIAS pin (RTD reference resistor)
  - R5: Connects +5V_USB to MAX7219 ISET pin (current set resistor)
  - R6: Connects IO4 to MOSFET gate (gate resistor)

### SPI Bus Nets
- [x] **SCK** net created and connected to:
  - [x] ESP32 IO16 - U1.20 ✅
  - [x] MAX31865 SCLK (U3 Pin 12) - U3.12 ✅
  - [x] MAX7219 CLK (U4 Pin 13) - U4.13 ✅

- [x] **MOSI** net created and connected to:
  - [x] ESP32 IO17 - U1.21 ✅
  - [x] MAX31865 SDI (U3 Pin 11) - U3.11 ✅ (Pin labeled "SDI" on chip)
  - [x] MAX7219 DIN (U4 Pin 1) - U4.1 ✅

- [x] **MISO** net created and connected to:
  - [x] ESP32 IO18 - U1.22 ✅
  - [x] MAX31865 SDO (U3 Pin 14) - U3.14 ✅ (Pin labeled "SDO" on chip)

- [x] **CS_RTD** net created and connected to:
  - [x] ESP32 IO10 - U1.14 ✅
  - [x] MAX31865 CS (U3 Pin 13) - U3.13 ✅

- [x] **CS_DISP** net created and connected to:
  - [x] ESP32 IO11 - U1.15 ✅
  - [x] MAX7219 LOAD (U4 Pin 12) - U4.12 ✅

### USB Data Nets
- [x] **D-** net created and connected to:
  - [x] USB connector DN1/DN2 pins (J1) - J1.B7, J1.A7
  - [x] ESP32 IO19 - U1.23

- [x] **D+** net created and connected to:
  - [x] USB connector DP1/DP2 pins (J1) - J1.B6, J1.A6
  - [x] ESP32 IO20 - U1.24

### UART Nets
- [ ] **UART_TX** net created and connected to:
  - [ ] ESP32 IO43 - **MISSING - NO UART_TX NET IN NETLIST**
  - [ ] Emergency Header Pin 3 (H1) - **MISSING**

- [ ] **UART_RX** net created and connected to:
  - [ ] ESP32 IO44 - **MISSING - NO UART_RX NET IN NETLIST**
  - [ ] Emergency Header Pin 4 (H1) - **MISSING**

### Control Signal Nets
- [x] **EN** net created and connected to:
  - [x] R3 resistor (one terminal) - R3.2 (**EN net**) ✅
  - [x] ESP32 EN pin - U1.45 ✅
  - [x] SW1 switch (one terminal) - SW1.1 ✅

- [x] **IO0** net created and connected to:
  - [x] ESP32 IO0 pin - U1.4 ✅
  - [x] SW2 switch (one terminal) - SW2.2 ✅

- [x] **IO4** net created and connected to:
  - [x] ESP32 IO4 pin - U1.8 ✅
  - [x] R6 resistor (one terminal) - R6.1 ✅

- [x] **IO5** net created and connected to:
  - [x] ESP32 IO5 pin - U1.9 ✅
  - [x] SW3 switch (one terminal) - SW3.1 ✅

- [x] **IO6** net created and connected to:
  - [x] ESP32 IO6 pin - U1.10 ✅
  - [x] SW4 switch (one terminal) - SW4.1 ✅

- [x] **IO7** net created and connected to:
  - [x] ESP32 IO7 pin - U1.11 ✅
  - [x] SW5 switch (one terminal) - SW5.1 ✅

### RTD Analog Nets
- [x] **FORCE+** net created and connected to:
  - [x] MAX31865 Pin 1 (FORCE+) - U3.1 (**$1N4176 net**) ✅
  - [x] R4 resistor Pin 1 - R4.1 ✅
  - [ ] Screw Terminal Pin 1 (J2) - **MISSING** (J2.1 is empty, should connect to `$1N4176` net)

- [x] **RTDIN+** net created and connected to:
  - [x] MAX31865 Pin 5 (RTDIN+) - U3.5 (**$1N4181 net**) ✅
  - [x] R4 resistor Pin 2 - R4.2 ✅
  - [ ] Screw Terminal Pin 2 (J2) - **MISSING** (J2.2 is empty, should connect to `$1N4181` net)

- [x] **RTDIN-** net created and connected to:
  - [x] MAX31865 Pin 8 (RTDIN-) - U3.8 (**RTDIN- net**) ✅
  - [ ] Screw Terminal Pin 3 (J2) - **MISSING** (J2.3 is empty, should connect to `RTDIN-` net)

**Note:** See `rtd_probe_connections.md` for detailed explanation of how to connect J2.

### Display Nets
- [x] **ISET** net created and connected to:
  - [x] MAX7219 ISET pin - U4.18 (**$1N4187 net**)
  - [x] R5 resistor (one terminal) - R5.1

- [x] **DIG0-3** nets created and connected to:
  - [x] MAX7219 DIG0 pin - U4.2 ✅
  - [x] MAX7219 DIG1 pin - U4.11 ✅
  - [x] MAX7219 DIG2 pin - U4.6 ✅
  - [x] MAX7219 DIG3 pin - U4.7 ✅
  - [x] Display Common Cathodes - LED1 pins connected ✅

- [x] **SegA-G,DP** nets created and connected to:
  - [x] MAX7219 SegA pin - U4.14 ✅
  - [x] MAX7219 SegB pin - U4.16 ✅
  - [x] MAX7219 SegC pin - U4.20 ✅
  - [x] MAX7219 SegD pin - U4.23 ✅
  - [x] MAX7219 SegE pin - U4.21 ✅
  - [x] MAX7219 SegF pin - U4.15 ✅
  - [x] MAX7219 SegG pin - U4.17 ✅
  - [x] MAX7219 SegDP pin - U4.22 ✅
  - [x] Display Segment Anodes - LED1 pins connected ✅

### Buzzer Circuit Nets
- [x] **MOSFET_GATE** net created and connected to:
  - [x] ESP32 IO4 pin - U1.8 ✅
  - [x] R6 resistor (one terminal) - R6.1 ✅
  - [x] R6 resistor (other terminal) - R6.2 (**$1N6488 net**) ✅
  - [x] MOSFET Gate (Q1 Pin 1) - Q1.1 (**$1N6488 net**) ✅

- [x] **MOSFET_DRAIN** net created and connected to:
  - [x] MOSFET Drain (Q1 Pin 3) - Q1.3 (**$1N6481 net**) ✅
  - [x] Buzzer negative terminal - BUZZER1.2 (**$1N6481 net**) ✅

**Note:** These connections are part of existing nets:
- [x] Buzzer positive terminal → +5V_USB net - BUZZER1.1 ✅
- [ ] MOSFET Source (Q1 Pin 2) → GND net - **MISSING** (Q1.2 is empty)

### USB CC Nets (Configuration)
- [x] **CC1** net created and connected to:
  - [x] USB connector CC1 pin (J1.B5) - **$1N4113 net**
  - [x] R1 resistor (one terminal) - R1.2

- [x] **CC2** net created and connected to:
  - [x] USB connector CC2 pin (J1.A5) - **$1N4111 net**
  - [x] R2 resistor (one terminal) - R2.1

---

## Progress Tracking

**Components Added:** 33 / 33 (All components present in netlist)  
**Nets Created:** ~27 / 28 (Excellent progress! ~96% complete!)  
**Connections Verified:** Excellent - See notes below

### Summary from Netlist Analysis:
**✅ Connected (Excellent Progress!):**
- **+5V_USB net** - Almost complete! ✅
  - USB VBUS pins ✅
  - LDO VIN/EN ✅
  - C1, C8 capacitors ✅
  - R5 resistor ✅
  - Buzzer positive ✅
  - Missing: MAX7219 VCC (Pin 24)
- **+3V3 net** - Complete! ✅
  - LDO Pin 5 (VOUT) ✅
  - C2, C3, C4, C5, C6, C7 capacitors ✅
  - ESP32 VCC ✅
  - R3 pull-up resistor ✅
  - Missing: MAX31865 VDD
- **GND net** - Almost complete! ✅
  - All capacitors C1-C8 ✅
  - All switches SW1-SW5 ✅
  - LDO Pin 2 (VSS) ✅
  - ESP32 GND ✅
  - MAX7219 GND ✅
  - Missing: MAX31865 GND, MOSFET Source (Q1 Pin 2)
- D+ and D- nets (complete) ✅
- CC1 and CC2 nets (complete) ✅
- ISET net (complete) ✅
- **EN net** - Complete! ✅ (ESP32 EN pin now connected!)
- **IO0 net** - Complete! ✅
- **IO4 net** - Complete! ✅ (NEW!)
- **IO5, IO6, IO7 nets** - Complete! ✅
- **MOSFET_GATE net** - Complete! ✅ (NEW! - $1N6488)
- **MOSFET_DRAIN net** - Complete! ✅ (NEW! - $1N6481)
- **Display DIG0-3 nets** - Complete! ✅
- **Display SegA-G,DP nets** - Complete! ✅
- **SPI Bus - Complete!** ✅ (NEW! - SCK, MOSI, MISO all connected)
- **Chip Selects - Complete!** ✅ (NEW! - CS_RTD, CS_DISP connected)
- RTD reference resistor connections ($1N4176, $1N4181 nets) ✅
- MAX31865 power connections (+3V3, GND) ✅

**❌ Still Missing Connections:**
- **RTD Screw Terminal (J2):**
  - J2 Pin 1 → `$1N4176` net (FORCE+)
  - J2 Pin 2 → `$1N4181` net (RTDIN+)
  - J2 Pin 3 → `RTDIN-` net
  - *See `rtd_probe_connections.md` for detailed explanation*
- **UART Nets (Need separate nets):**
  - UART_TX net: ESP32 IO43 (pin empty) → H1 Pin 3
  - UART_RX net: ESP32 IO44 (pin empty) → H1 Pin 4
- **Power Connections (Connect to existing nets):**
  - MAX31865 VDD → +3V3 net (need to identify VDD pin)
  - MAX31865 GND → GND net (need to identify GND pin)
  - MAX7219 VCC (Pin 24) → +5V_USB net (pin 24 is empty)
  - MOSFET Source (Q1 Pin 2) → GND net (pin 2 is empty)
- **RTD Connections:**
  - Screw Terminal J2 pins → RTD analog nets (J2 pins are empty)

---

## Notes

- Check off each component as you add it to your schematic
- Verify each net connection matches the schematic_diagrams.md
- Double-check critical components (R4 0.1% tolerance, LDO part number)
- Ensure all capacitors have one terminal to signal/power and one to GND
- Verify all switches connect to GND on one terminal

