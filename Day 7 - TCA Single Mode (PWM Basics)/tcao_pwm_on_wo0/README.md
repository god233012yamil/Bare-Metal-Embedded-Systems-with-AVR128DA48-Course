# AVR128DA48 — TCA0 Single-Slope PWM

Bare-metal C example for the **Microchip AVR128DA48** demonstrating how to generate a configurable PWM signal using **Timer/Counter A (TCA0)** in Single mode with Single-Slope PWM. No MCC, START, or ASF libraries are used.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Clock Configuration](#clock-configuration)
- [PWM Output Pin](#pwm-output-pin)
- [Project Structure](#project-structure)
- [How It Works](#how-it-works)
- [API Reference](#api-reference)
- [Demo Behavior](#demo-behavior)
- [Flowchart](#flowchart)
- [Customization](#customization)
- [Building with Microchip Studio](#building-with-microchip-studio)
- [License](#license)

---

## Overview

This project shows how to:

- Configure the **OSCHF** internal oscillator to 24 MHz with a /2 prescaler (CLK_PER = 12 MHz).
- Set up **TCA0** in Single mode with Single-Slope PWM on the **WO0** output.
- Dynamically change the **PWM frequency (Hz)** and **duty cycle (%)** at runtime via a simple function call.
- Automatically select the optimal **timer prescaler** to achieve the requested frequency within the 16-bit counter range.

---

## Hardware Requirements

| Component | Details |
|-----------|---------|
| MCU | AVR128DA48 (or compatible AVR DA-series device) |
| IDE | Microchip Studio 7 (formerly Atmel Studio) |
| Toolchain | AVR-GCC (bundled with Microchip Studio) |
| PWM Output | PA0 (WO0 default route — see [PWM Output Pin](#pwm-output-pin)) |

> **No external components are required** for the PWM output itself. Connect an oscilloscope or logic analyzer to the WO0 pin to observe the signal.

---

## Clock Configuration

| Parameter | Value |
|-----------|-------|
| Oscillator | OSCHF (internal high-frequency) |
| OSCHF Frequency | 24 MHz |
| Main Prescaler | /2 |
| **CLK_PER** | **12 MHz** |

The `CLK_PER_HZ` macro in `main.c` must match your actual peripheral clock. Update it if you modify `clock_init_24mhz_presc2()`.

```c
#define CLK_PER_HZ (12000000UL)
```

---

## PWM Output Pin

By default, TCA0 WO0 is routed to **PORTA PIN0 (PA0)** using the `PORTMUX_TCA0_PORTA_gc` setting.

```c
#define PWM_WO0_PORT   PORTA
#define PWM_WO0_PIN_bm PIN0_bm
```

If your board requires a different routing, modify:
1. `PORTMUX.TCAROUTEA` in `tca0_pwm_route_wo0_default()`
2. The `PWM_WO0_PORT` and `PWM_WO0_PIN_bm` macros

Refer to the **AVR128DA48 datasheet, PORTMUX section** for all available WO0 routes.

---

## Project Structure

```
avr128da48-tca0-pwm/
├── main.c          # Full bare-metal implementation
└── README.md       # This file
```

> This is a single-file bare-metal project. Open it directly in Microchip Studio as a **C Executable Project** targeting the AVR128DA48.

---

## How It Works

### 1. Clock Initialization

`clock_init_24mhz_presc2()` uses CCP-protected writes to configure:
- Main clock source → OSCHF
- OSCHF frequency → 24 MHz
- Main prescaler → /2 → CLK_PER = 12 MHz

### 2. TCA0 Initialization

`tca0_pwm_init()`:
- Routes WO0 outputs via PORTMUX
- Sets the WO0 pin as an output
- Configures TCA0 SINGLE mode with Single-Slope PWM waveform
- Enables CMP0 (Compare channel 0) to drive WO0
- Resets CNT, PER, and CMP0 to known values

### 3. Prescaler Selection

`tca0_pwm_pick_prescaler()` iterates through all available TCA clock dividers (DIV1 through DIV1024) and selects the **smallest prescaler** where:

```
TOP = (CLK_PER / prescaler / freq_hz) - 1
```

fits within the 16-bit PER register (≤ 0xFFFF).

### 4. PWM Update

`tca0_pwm_set()`:
- Validates inputs
- Calls the prescaler picker
- Computes `CMP0 = (TOP + 1) × duty% / 100`
- Stops the timer, updates PER and CMP0, then restarts with the new prescaler

In **Single-Slope PWM**:
- The counter counts up from 0 to PER (TOP), then resets.
- WO0 is **set** when CNT = 0 and **cleared** when CNT = CMP0.
- `duty = 0%` → CMP0 = 0 (output always low)
- `duty = 100%` → CMP0 = TOP + 1 (output always high)

---

## API Reference

```c
// Initialize the system clock (OSCHF @ 24 MHz, prescaler /2 → CLK_PER = 12 MHz)
static void clock_init_24mhz_presc2(void);

// Initialize TCA0 for Single-Slope PWM on WO0
static void tca0_pwm_init(void);

// Set PWM frequency and duty cycle at runtime
// freq_hz      : desired output frequency in Hz
// duty_percent : duty cycle 0..100 (clamped automatically)
static void tca0_pwm_set(uint32_t freq_hz, uint8_t duty_percent);
```

---

## Demo Behavior

The `main()` function demonstrates runtime PWM control:

1. **Startup**: Sets 20 kHz at 50% duty cycle.
2. **Sweep loop**: Sweeps duty cycle from 10% to 90% (in 10% steps) at **5 kHz**, with a software delay between each step.
3. **Frequency switch**: Changes to **1 kHz at 50%** with a longer hold time, then repeats.

> The delay loop is a crude busy-wait (`nop` loop). Replace it with a proper 1 ms tick timer for production use.

---

## Flowchart

See [`FLOWCHART.md`](FLOWCHART.md) for the full Mermaid diagram of the program flow.

---

## Customization

| Goal | What to Change |
|------|---------------|
| Different CLK_PER | Update `CLK_PER_HZ` and `clock_init_24mhz_presc2()` |
| Different WO output (WO1–WO5) | Change `CTRLB` CMP enable bit and PORTMUX routing |
| Different output pin | Update `PWM_WO0_PORT`, `PWM_WO0_PIN_bm`, and PORTMUX |
| Fixed frequency, variable duty only | Call `tca0_pwm_set()` with the same `freq_hz` each time |
| Interrupt-driven PWM update | Add TCA OVF interrupt handler and enable `INTCTRL` |

---

## Building with Microchip Studio

1. Open **Microchip Studio 7**.
2. Select **File → New → Project → GCC C Executable Project**.
3. Choose **AVR128DA48** as the target device.
4. Replace the generated `main.c` with the one from this repository.
5. Build the project (**F7**) and program your device via UPDI (e.g., Curiosity Nano, MPLAB PICkit 4, or Atmel-ICE).

---

## License

This project is released under the **MIT License**. You are free to use, modify, and distribute it in personal and commercial projects. See `LICENSE` for details.
