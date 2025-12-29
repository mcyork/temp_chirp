# RTD Probe Header (J2) Connection Guide

## Understanding the 3-Wire RTD Connection

The RTD (Resistance Temperature Detector) probe has **3 wires** that connect to the screw terminal J2. These wires connect to specific pins on the MAX31865 chip.

---

## Current Status from Netlist

**MAX31865 (U3) Connected Pins (from your netlist):**
- Pin 1 → `$1N4176` net
- Pin 5 → `$1N4181` net
- Pin 8 → `RTDIN-` net ✅
- Pin 10, 16 → `GND` net ✅
- Pin 20 → `+3V3` net ✅
- Pin 11 → `MOSI` net ✅
- Pin 12 → `SCK` net ✅
- Pin 13 → `CS_RTD` net ✅
- Pin 14 → `MISO` net ✅

**R4 Resistor (430Ω, 0.1%):**
- Pin 1 → `$1N4176` net
- Pin 2 → `$1N4181` net

**⚠️ ISSUE IDENTIFIED:** According to the MAX31865 datasheet and your schematic image:
- **Pin 1 should be BIAS** (not FORCE+)
- **Pin 2 should be REFIN+** (reference input positive)
- **Pin 7 should be RTDIN+** (RTD input positive)
- **Pin 8 is RTDIN-** ✅ (correct)

**Correct R4 Connection:**
- R4 should connect between **Pin 1 (BIAS)** and **Pin 2 (REFIN+)**
- Your current netlist shows R4 between Pin 1 and Pin 5, which is incorrect

**Screw Terminal J2:**
- Pin 1 → **EMPTY** (should connect to **MAX31865 Pin 7** - RTDIN+)
- Pin 2 → **EMPTY** (should connect to **MAX31865 Pin 8** - RTDIN-)  
- Pin 3 → **EMPTY** (should connect to **GND**)

---

## Correct MAX31865 Pinout (from datasheet and your schematic image)

**Pin Assignments:**
- **Pin 1 = BIAS** (Bias voltage output)
- **Pin 2 = REFIN+** (Reference input positive)
- **Pin 7 = RTDIN+** (RTD input positive)
- **Pin 8 = RTDIN-** (RTD input negative) ✅

**Correct R4 Connection:**
- R4 connects between **Pin 1 (BIAS)** and **Pin 2 (REFIN+)**
- This is the reference resistor for the ADC

## How to Connect J2 (Screw Terminal)

### Correct Connection Map:

```
RTD Probe Wire 1 ──── J2 Pin 1 ──── RTDIN+ Net ──── MAX31865 Pin 7 (RTDIN+)

RTD Probe Wire 2 ──── J2 Pin 2 ──── RTDIN- Net ──── MAX31865 Pin 8 (RTDIN-)

RTD Probe Wire 3 ──── J2 Pin 3 ──── GND Net ──── GND

R4 (430Ω) ──── MAX31865 Pin 1 (BIAS) ──── MAX31865 Pin 2 (REFIN+)
```

---

## Step-by-Step Connection Instructions

### ⚠️ FIRST: Fix R4 Connection
**R4 should connect between Pin 1 (BIAS) and Pin 2 (REFIN+), NOT Pin 1 and Pin 5**

In your schematic tool:
1. **Disconnect R4 from Pin 5**
2. **Connect R4 Pin 2 to MAX31865 Pin 2 (REFIN+)**
3. **Keep R4 Pin 1 connected to MAX31865 Pin 1 (BIAS)**

### Step 1: Connect J2 Pin 1
- **J2 Pin 1** → Connect to **MAX31865 Pin 7 (RTDIN+)**
- Create a new net called `RTDIN+` or use existing net
- **What this does:** This is one sense wire that measures voltage at one end of the RTD probe

### Step 2: Connect J2 Pin 2
- **J2 Pin 2** → Connect to **MAX31865 Pin 8 (RTDIN-)**
- This net already exists: `RTDIN-` ✅
- **What this does:** This is the other sense wire that measures voltage at the other end of the RTD probe

### Step 3: Connect J2 Pin 3
- **J2 Pin 3** → Connect to **GND net**
- **What this does:** This is the ground/return wire for the RTD probe

---

## Understanding 3-Wire RTD Operation

**Why 3 wires?**
- The RTD probe changes resistance with temperature
- Wire resistance adds error to the measurement
- 3-wire configuration compensates for wire resistance

**How it works:**
1. **BIAS** (Pin 1): Provides bias voltage output
2. **REFIN+** (Pin 2): Reference input positive - connects to other side of R4
3. **RTDIN+** (Pin 7): Measures voltage at one end of RTD via Wire 1
4. **RTDIN-** (Pin 8): Measures voltage at other end of RTD via Wire 2
5. **GND** (J2 Pin 3): Ground return via Wire 3

**The 430Ω resistor (R4):**
- Connects between **BIAS (Pin 1)** and **REFIN+ (Pin 2)**
- This is the **reference resistor** for the ADC measurement
- **CRITICAL:** Must be 0.1% tolerance for accurate temperature readings
- The MAX31865 compares RTD resistance to this 430Ω reference

---

## Visual Connection Summary

```
                    RTD Probe (3-Wire)
                         │
        ┌────────────────┼────────────────┐
        │                │                │
    Wire 1           Wire 2           Wire 3
        │                │                │
        ▼                ▼                ▼
    J2 Pin 1         J2 Pin 2         J2 Pin 3
        │                │                │
        │                │                │
        ▼                ▼                ▼
   $1N4176 Net      $1N4181 Net      RTDIN- Net
        │                │                │
        │                │                │
        ├─── R4.1 ──── R4.2 ───────────────┤
        │    (430Ω)                        │
        │                                   │
        ▼                                   ▼
  MAX31865 Pin 1                    MAX31865 Pin 8
  (FORCE+)                          (RTDIN-)
        │
        │
        ▼
  MAX31865 Pin 5
  (RTDIN+)
```

---

## What You Need to Fix

### ⚠️ CRITICAL: Fix R4 Connection First!

**Current (WRONG):**
- R4 Pin 1 → Pin 1 (BIAS) ✅
- R4 Pin 2 → Pin 5 ❌ (WRONG - Pin 5 is not REFIN+)

**Correct:**
- R4 Pin 1 → Pin 1 (BIAS) ✅
- R4 Pin 2 → Pin 2 (REFIN+) ✅

### Then Connect J2 - Direct Pin Connections:

1. **J2 Pin 1** → **MAX31865 Pin 7** (RTDIN+)
   - Just connect these two pins together

2. **J2 Pin 2** → **MAX31865 Pin 8** (RTDIN-)
   - Just connect these two pins together

3. **J2 Pin 3** → **GND**
   - Connect to any GND net/pin

---

## Summary - Direct Pin Connections

**J2 Screw Terminal Connections (Simple Version):**
```
J2 Pin 1  →  MAX31865 Pin 7  (RTDIN+)
J2 Pin 2  →  MAX31865 Pin 8  (RTDIN-)
J2 Pin 3  →  GND
```

**R4 Resistor Connection:**
```
R4 Pin 1  →  MAX31865 Pin 1  (BIAS)
R4 Pin 2  →  MAX31865 Pin 2  (REFIN+)
```

**Don't worry about net names** - just connect the pins directly as shown above!

