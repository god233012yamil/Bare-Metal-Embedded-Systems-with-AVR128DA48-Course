# PWM-Controlled Power Module

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![MCU](https://img.shields.io/badge/MCU-AVR128DA48-blue)](https://www.microchip.com/en-us/product/AVR128DA48)
[![DFP](https://img.shields.io/badge/DFP-AVR--Dx%202.4.286-blue)](https://packs.download.microchip.com/)
[![IDE](https://img.shields.io/badge/IDE-Atmel%20Studio%207-red)](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)
[![Toolchain](https://img.shields.io/badge/Toolchain-AVR--GCC-green)](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

A production-quality embedded firmware project for the **AVR128DA48 Curiosity Nano** development board. The firmware reads an analogue input (potentiometer or sensor), drives a high-frequency PWM output through a proportional control loop, and monitors a configurable threshold using the on-chip Analog Comparator — all without a single `delay_ms()` call.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware](#hardware)
  - [Target Board](#target-board)
  - [Pin Assignment](#pin-assignment)
  - [Block Diagram](#block-diagram)
- [Architecture](#architecture)
  - [Layer Separation](#layer-separation)
  - [Module Descriptions](#module-descriptions)
  - [Control-Loop State Machine](#control-loop-state-machine)
  - [Cooperative Scheduler](#cooperative-scheduler)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Building the Project](#building-the-project)
  - [Flashing to Hardware](#flashing-to-hardware)
  - [Serial Monitor](#serial-monitor)
- [Project Structure](#project-structure)
- [Configuration](#configuration)
- [Telemetry Output](#telemetry-output)
- [Design Decisions](#design-decisions)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

This project demonstrates a **full embedded application** using clean architecture principles on the AVR128DA48 microcontroller. It implements a PWM-controlled power output stage driven by an analogue feedback signal, with fast threshold detection via the Event System (EVSYS) and Analog Comparator (AC0).

The firmware is structured around three core design patterns:

| Pattern | Implementation |
|---|---|
| Cooperative scheduling | RTC PIT tick → task dispatch in super-loop |
| Event-driven response | AC0 interrupt + EVSYS autonomous triggering |
| Finite state machine | 4-state control loop with hysteresis |

---

## Features

- **ADC input** — 12-bit single-ended conversion on PD2 (potentiometer or external signal)
- **TCA0 PWM output** — Single-slope PWM at ≈ 23.4 kHz on PD0, 10-bit effective resolution
- **Proportional control loop** — ADC reading linearly mapped to PWM duty cycle
- **AC0 threshold detection** — Interrupt-driven comparator with configurable DACREF level
- **EVSYS fast path** — AC0 output event autonomously triggers ADC conversions with zero CPU latency
- **USART0 telemetry** — CSV data stream at 115200 baud over the on-board CDC-USB bridge
- **Cooperative scheduler** — RTC PIT at 1024 Hz, four non-blocking tasks, no OS required
- **FIFO ring buffer** — Lock-free ISR-safe receive buffer for USART RX
- **No `delay_ms()`** — All timing is tick-based; the CPU is never spin-waiting
- **Layered drivers** — Application layer never touches hardware registers directly

---

## Hardware

### Target Board

| Property | Value |
|---|---|
| MCU | AVR128DA48 |
| Board | [AVR128DA48 Curiosity Nano (EV35L43A)](https://www.microchip.com/en-us/development-tool/EV35L43A) |
| Clock | 24 MHz (OSCHF internal oscillator) |
| Supply | 3.3 V (USB) |
| DFP | AVR-Dx Device Pack 2.4.286 |
| IDE | Atmel Studio 7 / Microchip Studio |
| Toolchain | AVR-GCC (GCC C Executable Project) |

### Pin Assignment

| Pin | Peripheral | Direction | Function |
|-----|-----------|-----------|----------|
| PD2 | ADC0 AIN2 / AC0 AINP0 | Input (analogue) | Potentiometer / sensor input |
| PD0 | TCA0 WO0 | Output | PWM output (~23.4 kHz) |
| PA0 | USART0 TXD | Output | Serial telemetry → CDC-USB |
| PA1 | USART0 RXD | Input | Serial receive ← CDC-USB |
| PC6 | GPIO | Output | Heartbeat LED (active-low) |

> **Note:** PD2 is shared between ADC0 and AC0. Both peripherals coexist on the same pin because the digital input buffer is disabled (`PORT_ISC_INPUT_DISABLE_gc`), eliminating noise and leakage current.

### Block Diagram

```
  ┌──────────────────────────────────────────────────────────────┐
  │                      AVR128DA48                              │
  │                                                              │
  │  PD2 ──►  ADC0 ──────────────────► Control Loop             │
  │      └──► AC0  ──► EVSYS CH0 ──►  ADC0 START (autonomous)  │
  │                └──► ISR ──────────► Threshold flag           │
  │                                        │                     │
  │                                        ▼                     │
  │                                   Duty Calc ──► TCA0 ──► PD0│
  │                                                              │
  │  PA0 ◄── USART0 ◄── Telemetry task (CSV @ 500 ms)          │
  │  PC6 ◄── LED ◄────── Heartbeat task (toggle @ 500 ms)      │
  │                                                              │
  │  RTC PIT ──► ~1024 Hz tick ──► Scheduler ──► Tasks         │
  └──────────────────────────────────────────────────────────────┘
       │                                              │
      PD2                                            PD0
  [Potentiometer]                              [PWM Load / LED]
```

---

## Architecture

### Layer Separation

The firmware is strictly divided into three layers. The application layer **never** accesses hardware registers directly.

```
┌──────────────────────────────────────────────────────┐
│                  Application Layer                    │
│   main.c — state machine, task callbacks, telemetry  │
├──────────────────────────────────────────────────────┤
│                    Driver Layer                       │
│  adc │ tca │ ac │ usart │ evsys │ scheduler │ fifo   │
├──────────────────────────────────────────────────────┤
│                   Hardware Layer                      │
│         AVR128DA48 peripheral registers               │
└──────────────────────────────────────────────────────┘
```

### Module Descriptions

| Module | Files | Responsibility |
|--------|-------|----------------|
| **config** | `inc/config.h` | All compile-time constants, pin definitions, tuning parameters |
| **adc** | `src/adc.c`, `inc/adc.h` | ADC0 init, single-shot trigger, RESRDY ISR, result storage |
| **tca** | `src/tca.c`, `inc/tca.h` | TCA0 single-slope PWM init, glitch-free duty-cycle updates via CMP0BUF |
| **ac** | `src/ac.c`, `inc/ac.h` | AC0 init, DACREF threshold control, edge interrupt, software event flag |
| **usart** | `src/usart.c`, `inc/usart.h` | USART0 init, polled TX, interrupt-driven RX via FIFO |
| **evsys** | `src/evsys.c`, `inc/evsys.h` | EVSYS channel wiring: AC0_OUT → ADC0 START |
| **scheduler** | `src/scheduler.c`, `inc/scheduler.h` | RTC PIT tick, task table, cooperative dispatch |
| **fifo** | `src/fifo.c`, `inc/fifo.h` | Generic lock-free power-of-two ring buffer |
| **main** | `src/main.c` | System init, task registration, super-loop, control FSM |

### Control-Loop State Machine

The power module control loop is implemented as a four-state FSM, advanced one step per 100 ms task invocation.

```
       ┌─────────┐
 reset │         │
  ───► │  IDLE   │ ◄─────────────────────────────────┐
       │  (0)    │                                    │
       └────┬────┘                                    │
            │ first run                               │
            ▼                                         │
       ┌──────────┐      ADC not ready           ┌────┴─────┐
       │ SAMPLING │ ─────────────────────────── ►│ SAMPLING │
       │   (1)    │ ◄──────────────────────────  │  (loop)  │
       └────┬─────┘                               └──────────┘
            │ ADC result ready
            ▼
       ┌───────────┐
       │ ADJUSTING │  ── compute duty = adc >> 2
       │    (2)    │  ── TCA_SetDuty(duty)
       └─────┬─────┘
             │ duty applied
             ▼
       ┌──────────┐  AC fires HIGH ──► clamp duty=MAX, log event
       │ RUNNING  │ ─────────────────────────────────────────────►
       │   (3)    │                                              │
       └────┬─────┘  AC fires LOW  ──► resume normal control    │
            │ next tick                                          │
            └──────────────────────────────────────────────────►┘
                                 (back to SAMPLING)
```

### Cooperative Scheduler

All tasks are non-blocking and run to completion before the scheduler returns control to the super-loop. Task periods are multiples of the ~1 ms RTC PIT tick.

| Task | Period | Action |
|------|--------|--------|
| `Task_ADC` | ~50 ms | Triggers an ADC conversion when the FSM is in `SAMPLING` state |
| `Task_Control` | ~100 ms | Advances the control-loop FSM by one step |
| `Task_Telemetry` | ~500 ms | Emits one CSV record over USART0 |
| `Task_LED` | ~500 ms | Toggles PC6 heartbeat LED |

---

## Getting Started

### Prerequisites

- **Atmel Studio 7** or **Microchip Studio** (Windows)
- **AVR-Dx Device Family Pack** version 2.4.286 or later  
  *(Tools → Device Pack Manager → search "AVR-Dx")*
- **AVR128DA48 Curiosity Nano** board (EV35L43A)
- A potentiometer or signal source connected to **PD2** (optional — the firmware runs without it, outputting 0 % duty)
- A serial terminal (e.g. [PuTTY](https://www.putty.org/), [CoolTerm](https://freeware.the-meiers.org/), or the Microchip Data Visualizer) for telemetry

### Building the Project

1. Clone or download this repository.
2. Open Atmel Studio 7 / Microchip Studio.
3. **File → Open → Project/Solution** → select `PWM_Power_Module.cproj`.
4. Verify the device pack path is resolved (no red markers in Solution Explorer).
5. Select the desired configuration (**Debug** or **Release**) from the toolbar.
6. Press **F7** (Build Solution).

The build output should report `0 errors`.

### Flashing to Hardware

1. Connect the Curiosity Nano board via USB.
2. In Atmel Studio: **Debug → Start Without Debugging** (Ctrl+Alt+F5) to flash and run, or **Debug → Start Debugging and Break** (F5) to flash and halt at `main`.
3. The on-board PKOB4 (Nano debugger) handles programming automatically — no external programmer is needed.

### Serial Monitor

The firmware streams telemetry over **USART0** routed to the on-board CDC-USB bridge.

| Setting | Value |
|---------|-------|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

Open your serial terminal and select the COM port assigned to the Curiosity Nano CDC interface. You should see:

```
PWM Power Module v1.0 — AVR128DA48
Format: tick,adc,duty,ac_out,state
512,2048,512,0,3
1024,2100,525,0,3
1536,2050,512,0,3
...
```

---

## Project Structure

```
PWM_Power_Module/
│
├── PWM_Power_Module.cproj      # Atmel Studio 7 project file
│
├── inc/                        # Header files (public interfaces)
│   ├── config.h                # Global configuration & constants
│   ├── adc.h                   # ADC0 driver interface
│   ├── tca.h                   # TCA0 PWM driver interface
│   ├── ac.h                    # AC0 comparator driver interface
│   ├── usart.h                 # USART0 driver interface
│   ├── evsys.h                 # Event System configuration interface
│   ├── scheduler.h             # Cooperative scheduler interface
│   └── fifo.h                  # Ring buffer interface
│
├── src/                        # Source files (implementations)
│   ├── main.c                  # Application entry point & control FSM
│   ├── adc.c                   # ADC0 driver + RESRDY ISR
│   ├── tca.c                   # TCA0 PWM driver
│   ├── ac.c                    # AC0 comparator driver + AC ISR
│   ├── usart.c                 # USART0 driver + RXC ISR
│   ├── evsys.c                 # Event System channel wiring
│   ├── scheduler.c             # Scheduler + RTC PIT ISR
│   └── fifo.c                  # Lock-free ring buffer
│
├── README.md                   # This file
├── FLOWCHART.md                # System flowcharts (Mermaid)
├── CHANGELOG.md                # Version history
├── CONTRIBUTING.md             # Contribution guidelines
└── LICENSE                     # MIT License
```

---

## Configuration

All tunable parameters are centralised in `inc/config.h`. No other file should contain magic numbers.

| Symbol | Default | Description |
|--------|---------|-------------|
| `F_CPU_HZ` | `24000000` | CPU clock frequency (Hz) |
| `SCHED_TICK_HZ` | `1024` | Scheduler tick rate (Hz) |
| `TASK_ADC_PERIOD_TICKS` | `51` | ADC task period (~50 ms) |
| `TASK_CTRL_PERIOD_TICKS` | `102` | Control task period (~100 ms) |
| `TASK_TELEM_PERIOD_TICKS` | `512` | Telemetry task period (~500 ms) |
| `TASK_LED_PERIOD_TICKS` | `512` | LED task period (~500 ms) |
| `ADC_MUX_INPUT` | `ADC_MUXPOS_AIN2_gc` | ADC positive input channel |
| `TCA0_PERIOD` | `1023` | PWM top value (sets frequency) |
| `AC_DACREF_DEFAULT` | `128` | AC threshold (~50 % of VDD) |
| `USART_BAUD_RATE` | `115200` | USART baud rate |
| `FIFO_RX_SIZE` | `64` | USART RX buffer size (must be power of 2) |
| `CTRL_HIGH_THRESHOLD` | `3072` | ADC count for "high-power" state (~75 %) |
| `CTRL_LOW_THRESHOLD` | `1024` | ADC count for "low-power" state (~25 %) |
| `CTRL_ADC_TO_PWM_SHIFT` | `2` | Right-shift to map 12-bit ADC → 10-bit PWM |

---

## Telemetry Output

Each line emitted over USART0 is a comma-separated record:

```
tick,adc,duty,ac_out,state
```

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `tick` | uint16 | 0–65535 | Scheduler tick counter (wraps) |
| `adc` | uint16 | 0–4095 | Latest 12-bit ADC result |
| `duty` | uint16 | 0–1023 | Applied PWM duty count |
| `ac_out` | bool | 0 / 1 | AC0 CMPSTATE: 1 = above threshold |
| `state` | uint8 | 0–3 | FSM state (0=IDLE, 1=SAMPLING, 2=ADJUSTING, 3=RUNNING) |

---

## Design Decisions

**No RTOS, no `delay_ms()`** — A cooperative scheduler built on the RTC PIT provides deterministic task timing without the overhead of a real-time OS or blocking delays. Every task returns in microseconds.

**Double-buffered PWM updates** — The duty cycle is written to `TCA0.SINGLE.CMP0BUF`, which the hardware copies to `CMP0` on the next timer overflow. This eliminates output glitches that would occur from writing the active register mid-cycle.

**Lock-free FIFO** — The ring buffer uses `volatile` head/tail indices and the single-producer/single-consumer contract to avoid disabling interrupts in the RXC ISR. This keeps USART receive latency minimal.

**EVSYS fast path** — Wiring AC0_OUT to ADC0 START via Channel 0 means that a threshold crossing triggers an ADC conversion with hardware-only latency (no CPU involvement). The result is available to the next control task invocation regardless of scheduler timing.

**Shared PD2 for ADC and AC** — Both peripherals can use the same pin simultaneously when the digital input buffer is disabled. The analogue circuitry inside the chip routes the pin voltage to both the ADC sample-and-hold and the AC comparator input independently.

**`volatile` + `cli()/sei()` for shared data** — Multi-byte values read in the telemetry task (tick counter, ADC result, duty cycle) are snapshotted inside a critical section to prevent torn reads on the 8-bit AVR architecture.

---

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.
