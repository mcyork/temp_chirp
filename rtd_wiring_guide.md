# RTD Probe Wiring Guide

## Your 3-Wire RTD Probe

You have:
- **1 Red wire**
- **2 Blue wires**

## Screw Terminal J2 Connections

The screw terminal J2 has 3 pins. Connect your RTD probe wires as follows:

### Correct Wiring:

```
J2 Pin 1 (usually top)  →  Red wire    →  RTDIN+ (MAX31865 Pin 7)
J2 Pin 2 (middle)       →  Blue wire 1 →  RTDIN- (MAX31865 Pin 8)  
J2 Pin 3 (bottom)       →  Blue wire 2 →  GND
```

### Important Notes:

1. **Red wire** typically goes to **RTDIN+** (Pin 1 of J2)
2. **One blue wire** goes to **RTDIN-** (Pin 2 of J2)
3. **Other blue wire** goes to **GND** (Pin 3 of J2)

### If You're Not Sure Which Blue Wire Goes Where:

For a 3-wire RTD, it usually doesn't matter which blue wire goes to RTDIN- vs GND, but try:
- **First try:** Blue wire 1 → J2 Pin 2, Blue wire 2 → J2 Pin 3
- **If that doesn't work:** Swap the blue wires (Blue wire 1 → J2 Pin 3, Blue wire 2 → J2 Pin 2)

## What "8888" Means

"8888" on the display means the MAX31865 detected a fault. Common causes:
- RTD probe not connected
- Wrong wiring
- Open circuit (broken wire)
- Short circuit

## Testing Steps

1. **Disconnect all wires** - Display should show fault (8888)
2. **Connect Red wire to J2 Pin 1** - Still should show fault
3. **Connect one Blue wire to J2 Pin 2** - Still should show fault  
4. **Connect other Blue wire to J2 Pin 3** - Should now read temperature!

If it still shows 8888 after connecting all 3 wires, try swapping the two blue wires.
