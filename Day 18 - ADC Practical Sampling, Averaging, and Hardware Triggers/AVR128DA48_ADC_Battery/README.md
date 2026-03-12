# AVR128DA48 ADC Battery Voltage Measurement

## Overview

This project demonstrates how to use the ADC peripheral on the AVR128DA48
to measure battery voltage every 1 ms, using the event system for
autonomous, CPU-independent triggering.

## Use Case

**Measure battery voltage every 1 ms**

| Block | Role |
|-------|------|
| TCB0  | Generates a 1 kHz periodic event (1 ms period) |
| EVSYS | Routes TCB0 capture event → ADC0 START trigger |
| ADC0  | Single conversion per event, 12-bit result |
| ISR   | `ADC0_RESRDY_vect` stores result in circular buffer |
| Main  | Computes 16-sample running average → battery mV |

## Hardware

* **Board:** AVR128DA48 Curiosity Nano (EV35F40A)
* **Analog input:** AIN0 / PD0
* **LED (heartbeat):** PC6 (active LOW, on-board LED)

### Resistor Divider for Battery Measurement

Connect the battery through a resistor divider to keep the voltage
within 0 – VDD range:

```
Battery (+) ──┬── R1 (10 kΩ) ──┬── PD0 (AIN0)
               │                │
              GND            R2 (10 kΩ)
                                │
                               GND
```

With equal R1 = R2 the divider ratio is 2.0, so the measurable
battery range is 0 – 2×VDD (0 – 6.6 V for a 3.3 V board).

Edit `DIVIDER_RATIO` in `main.c` to match your actual resistor values.

## Configuration (`main.c`)

| Macro | Default | Description |
|-------|---------|-------------|
| `ADC_AVG_SAMPLES` | 16 | Samples averaged per result |
| `USE_INTERNAL_VREF` | 0 | 0 = VDD reference, 1 = 2.048 V internal |
| `ADC_MUXPOS_CHANNEL` | AIN0 | ADC input channel |
| `DIVIDER_RATIO` | 2.0 | Resistor divider multiplier |
| `VDD_MV` | 3300 | VDD in mV (used when `USE_INTERNAL_VREF=0`) |
| `INTERNAL_VREF_MV` | 2048 | Internal VREF in mV |

## Software Design

### TCB0 — 1 kHz Event Source

```
TCB_CLK = F_CPU / 2 = 4 MHz / 2 = 2 MHz
Period  = (CCMP + 1) / TCB_CLK = 2000 / 2 MHz = 1 ms  ✓
```

TCB0 is configured in **Periodic Interrupt mode** with CAPTEI output
enabled. On each compare match, TCB0 generates an event on the event bus.

### EVSYS — Event Routing

```
EVSYS.CHANNEL0     = TCB0_CAPT   (event generator)
EVSYS.USERADC0START = CHANNEL0   (event user: ADC start)
```

### ADC0 — Event-Triggered Single Conversion

- `STARTEI` bit enables event-triggered conversions.
- 12-bit resolution (default on AVR-Dx).
- ADC clock = F_CPU / 4 = 1 MHz (within the 0.15–2 MHz spec).
- `INITDLY = DLY32` provides 32 ADC clocks settling time after trigger.
- `RESRDY` interrupt fires when the conversion result is ready.

### ISR

Stores each 12-bit result into a power-of-2 circular buffer.

### Main Loop

Accumulates `ADC_AVG_SAMPLES` (16) raw results, then:

1. Divides to get the average raw ADC count.
2. Converts to mV using the reference voltage and divider ratio.
3. Stores the result in `battery_mv`.
4. Toggles the on-board LED as a heartbeat (~62.5 Hz at 16 samples).

Inspect `battery_mv` in the **Watch** window of the debugger.

## Building

1. Open `AVR128DA48_ADC_Battery.atsln` in **Atmel Studio 7** or
   **Microchip Studio**.
2. Ensure the **AVR-Dx Device Pack 2.4.286** is installed
   (*Tools → Device Pack Manager*).
3. Select **Debug** or **Release** configuration.
4. Press **F7** to build.
5. Connect the Curiosity Nano board and press **F5** to program and debug.

## Expected Behaviour

* The on-board LED (PC6) toggles roughly every 16 ms.
* In the debugger Watch window, `battery_mv` updates with the
  battery voltage in millivolts.
* Without any input on PD0 the ADC will float; connect a known
  voltage or tie PD0 to GND/VDD to verify correct readings.

## Notes

* The floating-point multiply in `adc_to_mv()` is acceptable here
  because it runs in the main loop at ~62.5 Hz, not in the ISR.
* For tighter memory budgets replace the `float` multiply with a
  fixed-point Q-format calculation.
* `libm` is linked by the project to support `float` math.
