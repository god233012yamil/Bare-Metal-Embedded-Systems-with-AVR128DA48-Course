# AVR128DA48 PWM Frequency Measurement

A bare-metal C project for the **AVR128DA48 Curiosity Nano** evaluation board that demonstrates PWM generation and hardware frequency measurement using TCA0, AC0, the Event System (EVSYS), and TCB0 — all without any software polling.

---

## Table of Contents

- [Overview](#overview)
- [Hardware Requirements](#hardware-requirements)
- [Hardware Connection](#hardware-connection)
- [Peripheral Architecture](#peripheral-architecture)
- [Signal Chain](#signal-chain)
- [Frequency Calculation](#frequency-calculation)
- [Project Structure](#project-structure)
- [Build & Flash Instructions](#build--flash-instructions)
- [Extending the Project](#extending-the-project)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Overview

| Item | Detail |
|------|--------|
| Target MCU | AVR128DA48 |
| Board | AVR128DA48 Curiosity Nano |
| IDE | Atmel Studio 7 / Microchip Studio |
| Device Pack | AVR-Dx Device Pack 2.4.286 |
| Project type | GCC C Executable Project |
| System clock | 4 MHz (internal oscillator, default) |

The firmware:

1. **Generates** a 1 kHz, 50 % duty-cycle PWM signal on **PA0** (TCA0 WO0).  
2. **Detects** rising edges of that signal using **AC0** (PA7 / AIN0 as positive input, internal DAC at VDD/2 as threshold).  
3. **Routes** AC0 output events through **EVSYS channel 0** to TCB0's capture input.  
4. **Measures** the PWM period in the **TCB0 capture ISR** and stores the computed frequency (Hz) in the global variable `g_pwm_frequency_hz`.

---

## Hardware Requirements

- AVR128DA48 Curiosity Nano board  
- One jumper wire (male-to-male)  
- USB cable (Micro-B) for programming and power

---

## Hardware Connection

> **Important:** This single wire is the only external connection needed.

| From | To | Purpose |
|------|----|---------|
| **PA0** (TCA0 WO0 – PWM output) | **PA7** (AC0 AIN0 – positive input) | Feed PWM signal into the analogue comparator |

Both pins are available on the Curiosity Nano's edge connector. Use a short jumper wire to connect them.

---

## Peripheral Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      AVR128DA48                              │
│                                                              │
│  ┌──────────┐  PWM on PA0     PA7 (AIN0)  ┌──────────────┐ │
│  │  TCA0    ├──────────────[wire]─────────►│  AC0         │ │
│  │ (1 kHz,  │                              │ (+ = AIN0    │ │
│  │  50% DC) │                              │  - = DAC=    │ │
│  └──────────┘                              │  VDD/2)      │ │
│                                            └──────┬───────┘ │
│                                                   │ event   │
│                                            ┌──────▼───────┐ │
│                                            │  EVSYS       │ │
│                                            │  Channel 0   │ │
│                                            └──────┬───────┘ │
│                                                   │ event   │
│                                            ┌──────▼───────┐ │
│                                            │  TCB0        │ │
│                                            │ (FRQMEAS)    │ │
│                                            └──────┬───────┘ │
│                                                   │ ISR     │
│                                          g_pwm_frequency_hz │
└─────────────────────────────────────────────────────────────┘
```

---

## Signal Chain

| Step | Peripheral | Action |
|------|-----------|--------|
| 1 | **TCA0** | Generates 1 kHz, 50 % duty PWM on PA0 using single-slope mode. PER = 249, CMP0 = 125, prescaler = DIV16 (timer clock = 250 kHz). |
| 2 | **AC0** | Compares PA7 (AIN0, positive) against internal DAC (VDD/2, negative). Output goes high when PA7 > VDD/2, i.e. when PWM is high. |
| 3 | **EVSYS** | Channel 0 generator = AC0 output; User = TCB0 capture input. Rising edge of AC0 output triggers a capture event in TCB0. |
| 4 | **TCB0** | Runs in Frequency Measurement (FRQMEAS) mode at F_CPU/2 = 2 MHz. Each rising edge event latches the counter into CCMP and resets the counter. The capture ISR reads CCMP and computes the frequency. |

---

## Frequency Calculation

```
TCB0 clock frequency  = F_CPU / 2 = 4,000,000 / 2 = 2,000,000 Hz
Captured ticks (CCMP) = ticks between two consecutive rising edges
PWM frequency (Hz)    = 2,000,000 / CCMP
```

**Expected result for 1 kHz PWM:**  
CCMP ≈ 2000 → measured frequency ≈ 1000 Hz

The result is stored in:

```c
volatile uint32_t g_pwm_frequency_hz;
```

---

## Project Structure

```
AVR128DA48_PWM_Freq_Measure/
├── AVR128DA48_PWM_Freq_Measure.atsln          ← Atmel/Microchip Studio solution
└── AVR128DA48_PWM_Freq_Measure/
    ├── AVR128DA48_PWM_Freq_Measure.cproj      ← Project file (device, toolchain)
    └── main.c                                 ← All source code
```

---

## Build & Flash Instructions

### 1. Open the Solution

1. Launch **Atmel Studio 7** or **Microchip Studio**.  
2. Go to **File → Open → Project/Solution…**  
3. Browse to `AVR128DA48_PWM_Freq_Measure.atsln` and click **Open**.

### 2. Verify Device Pack

Ensure **AVR-Dx Device Pack ≥ 2.4.286** is installed:

- **Tools → Device Pack Manager** → search for `AVR-Dx` → install/update if needed.

### 3. Build

- Press **F7** or go to **Build → Build Solution**.  
- Verify 0 errors in the Output window.

### 4. Connect the Curiosity Nano

- Connect the board via USB.  
- The on-board PKOB nano programmer is automatically detected.

### 5. Flash & Debug

- Press **F5** (Start Debugging) or **Ctrl+Alt+F5** (Start Without Debugging) to flash and run.

### 6. Verify Operation

Use the **I/O View** or a watch window in the debugger to inspect `g_pwm_frequency_hz`. With the PA0→PA7 wire connected and the firmware running, the variable should read approximately **1000**.

---

## Extending the Project

### Read frequency over USART

Add USART0 initialisation and transmit `g_pwm_frequency_hz` periodically in the `while(1)` loop using `printf` (retarget stdout) or a simple integer-to-ASCII formatter.

### Indicate lock with the LED

```c
/* In the while(1) loop */
if (g_pwm_frequency_hz >= 950UL && g_pwm_frequency_hz <= 1050UL)
    PORTC.OUTCLR = PIN6_bm;   /* LED on – in range */
else
    PORTC.OUTSET = PIN6_bm;   /* LED off – out of range */
```

### Change PWM frequency

Modify `TCA0_PER_VALUE` and `TCA0_CMP0_VALUE` in `main.c`:

```c
// Example: 500 Hz, 50 % duty cycle (timer clock = 250 kHz)
#define TCA0_PER_VALUE   499U
#define TCA0_CMP0_VALUE  250U
```

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| `g_pwm_frequency_hz` stays 0 | PA0 not connected to PA7 | Check the jumper wire |
| `g_pwm_frequency_hz` stays 0 | AC0 threshold wrong | Verify DAC value / VDD level |
| Measured frequency is double expected | AC0 triggering on both edges | Use AC output edge detect or filter in EVSYS (see data sheet §22) |
| Build error: unknown register | Wrong device pack | Install AVR-Dx Device Pack ≥ 2.4.286 |
| No CCMP capture interrupt | TCB0 event input not connected | Verify `EVSYS.USERTCB0CAPT` register value |

---

## License

This project is released under the **MIT License**. See [LICENSE](LICENSE) for details.

> *This example is provided for educational purposes. Always consult the AVR128DA48 data sheet and the AVR-Dx Device Pack documentation for production designs.*
