# RTD Wiring Verification Based on Schematic

## Schematic Analysis

From your schematic:
- **J2 Pin 1** → RTDIN+ net → **MAX31865 Pin 7 (RTDIN+)**
- **J2 Pin 2** → RTDIN- net → **MAX31865 Pin 8 (RTDIN-)**  
- **J2 Pin 3** → GND net → **MAX31865 Pin 9 (FORCE-)** and system GND

## Correct Wiring for Your 3-Wire RTD Probe

```
RTD Probe Wire Colors → J2 Terminal → MAX31865 Pin → Function
─────────────────────────────────────────────────────────────
Red wire            → Pin 1       → Pin 7 (RTDIN+) → RTD Input Positive
Blue wire 1         → Pin 2       → Pin 8 (RTDIN-) → RTD Input Negative
Blue wire 2         → Pin 3       → Pin 9 (FORCE-)  → Ground Return
```

## Current Problem

You're seeing **0277** on the display, which equals:
- RTD Register: 277
- Resistance: (277/32768) × 430Ω = **~3.6Ω**

This is **way too low** for a PT100 RTD, which should read:
- At 0°C: ~100Ω = RTD register ~7620
- At room temp (~20°C): ~108Ω = RTD register ~8220

## Troubleshooting Steps

### Step 1: Verify Physical Connections
1. **Disconnect all 3 wires from J2**
2. **Check display** - Should show fault (8888 or 90XX)
3. **Connect Red wire to J2 Pin 1** - Still should show fault
4. **Connect Blue wire 1 to J2 Pin 2** - Still should show fault
5. **Connect Blue wire 2 to J2 Pin 3** - Should now show reasonable reading

### Step 2: Try Swapping Blue Wires
If you still see very low readings (like 0277), try:
- **Swap the two blue wires:**
  - Blue wire 1 → J2 Pin 3 (GND)
  - Blue wire 2 → J2 Pin 2 (RTDIN-)

### Step 3: Check for Shorts
- Make sure wires aren't touching each other
- Ensure screws are tight but not shorting to adjacent terminals
- Check that the RTD probe itself isn't damaged

### Step 4: Expected Readings
- **Good reading:** 7000-8500 (70-85Ω at room temp)
- **Too low:** < 1000 (< 10Ω) - suggests short or wrong wiring
- **Too high:** > 20000 (> 200Ω) - suggests open circuit or wrong wiring
- **Fault:** 8888 or 90XX - sensor fault detected

## What to Look For

When properly connected, you should see:
- **Stable reading** around 7000-8500 (not flickering)
- **Reading changes** when you touch/warm the probe
- **No fault codes** (8888 or 90XX)

If you see 0277 or similar low values, the most likely causes are:
1. **Wires swapped** - Try swapping the blue wires
2. **Short circuit** - Check for wires touching
3. **Probe damaged** - Try a different RTD probe if available
