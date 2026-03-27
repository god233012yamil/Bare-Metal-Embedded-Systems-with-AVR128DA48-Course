# UART RX FIFO Buffer Lab
### AVR128DA48 Curiosity Nano — Atmel/Microchip Studio 7

---

## Overview

This project implements interrupt-driven UART receive buffering using a
hand-written circular FIFO (ring buffer) on the **AVR128DA48 Curiosity Nano**
development board.

| Goal | Achieved by |
|---|---|
| Receive UART data using an interrupt | `USART1_RXC_vect` ISR in `uart.c` |
| Store data in a FIFO buffer | `fifo.c` / `fifo.h` circular buffer |
| Process data in the main loop | `main.c` — `process_byte()` called each iteration |

---

## File Structure

```
UART_RX_FIFO/
├── UART_RX_FIFO.atsln          ← Atmel Studio solution (open this)
└── UART_RX_FIFO/
    ├── UART_RX_FIFO.cproj      ← Project file (device, toolchain, flags)
    ├── main.c                  ← Application entry point & super-loop
    ├── uart.h / uart.c         ← USART1 driver (ISR + polled TX)
    └── fifo.h / fifo.c         ← Generic circular FIFO implementation
```

---

## Hardware

**Board:** AVR128DA48 Curiosity Nano (DM164151)

The on-board nEDBG debugger exposes a **virtual COM port (CDC)** that is
wired directly to USART1 on the AVR128DA48:

| Signal | AVR128DA48 Pin | USART1 (default MUX) |
|--------|---------------|----------------------|
| TXD    | PC0           | USART1 TX            |
| RXD    | PC1           | USART1 RX            |

No external wiring is needed. Connect the board via USB; the CDC port
appears automatically on Windows (CDC driver) and Linux/macOS (ttyACM / cu).

---

## Serial Terminal Settings

| Parameter | Value |
|-----------|-------|
| Baud rate | **9600** |
| Data bits | 8 |
| Parity    | None |
| Stop bits | 1 |
| Flow ctrl | None |

Recommended terminals: **PuTTY**, **TeraTerm**, **CoolTerm**, `screen` (Linux/macOS).

---

## Clock Configuration

The project is configured for the **4 MHz internal oscillator** (reset default
of the AVR128DA48). `F_CPU=4000000UL` is defined as a preprocessor symbol in
the `.cproj` for both Debug and Release configurations.

If you change the system clock, update `F_CPU` in:
- **Project Properties → Toolchain → AVR/GNU C Compiler → Symbols**

The baud-rate register value is computed automatically from `F_CPU` and
`UART_BAUD_RATE` at compile time (see `uart.h`).

---

## Building & Programming

1. Open `UART_RX_FIFO.atsln` in **Atmel Studio 7** or **Microchip Studio**.
2. Confirm the **AVR-Dx Device Pack 2.4.286** is installed
   *(Tools → Device Pack Manager)*.
3. Press **F7** to build.
4. Connect the Curiosity Nano board.
5. Press **Start Without Debugging** (Ctrl+Alt+F5) to program and run,
   or use the MPLAB SNAP / on-board debugger for a debug session (F5).

---

## Expected Terminal Output

```
========================================
  AVR128DA48 – UART RX FIFO Buffer Lab
  USART1 @ 9600 baud, 8-N-1
  FIFO size: 64 bytes
========================================
Type characters and press Enter...

[FIFO] bytes in buffer: 1 / 63
H <- received printable: 'H' (0x48)
[FIFO] bytes in buffer: 1 / 63
i <- received printable: 'i' (0x69)
[FIFO] bytes in buffer: 0 / 63

>
```

---

## FIFO Design Notes

- **Type:** Single-producer / single-consumer lock-free circular buffer.
- **Size:** 64 bytes (`FIFO_BUFFER_SIZE` in `fifo.h`). Must remain a power of two.
- **Capacity:** 63 usable bytes — one slot is kept empty to distinguish *full* from *empty* without an extra counter.
- **Thread safety:** `head` is written only by the ISR; `tail` is written only by the main loop. 8-bit reads/writes on AVR are atomic, so no critical section is needed for index updates.
- **Overflow policy:** Newest byte is silently discarded when full. Add error handling in the ISR if lossless reception is required.

---

## Modifying Baud Rate or FIFO Size

**Baud rate** — change `UART_BAUD_RATE` in `uart.h`:
```c
#define UART_BAUD_RATE  115200UL
```

**FIFO size** — change `FIFO_BUFFER_SIZE` in `fifo.h` (must be a power of two):
```c
#define FIFO_BUFFER_SIZE  128u
```

---

## License

MIT — free to use, modify, and redistribute for educational and commercial
purposes. No warranty expressed or implied.
