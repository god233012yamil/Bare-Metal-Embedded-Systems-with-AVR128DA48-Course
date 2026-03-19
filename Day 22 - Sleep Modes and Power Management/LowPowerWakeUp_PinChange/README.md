# Low-Power Wake-Up via Pin Change Interrupt (SW0 / PC7)
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates **Power-Down sleep with a pin change interrupt as the sole wake-up source**.  
The MCU sleeps indefinitely. Each press of the on-board button **SW0 (PC7)** generates a falling-edge interrupt that wakes the CPU, toggles **LED0 (PC6)**, and returns immediately to sleep.

No timers, RTC, ADC, or EVSYS are used — only PORTC and SLPCTRL.

---

### Hardware
| Item | Pin | Detail |
|------|-----|--------|
| MCU  | — | AVR128DA48 |
| Board | — | AVR128DA48 Curiosity Nano |
| LED0 | PC6 | Active LOW |
| SW0  | PC7 | Active LOW, internal pull-up enabled |
| Programmer | — | On-board nEDBG (UPDI) |

---

### Software
| Item | Detail |
|------|--------|
| IDE | Atmel Studio 7 / Microchip Studio 7 |
| Project type | GCC C Executable Project |
| Device Pack | Microchip.AVR-Dx_DFP 2.4.286 |
| Optimisation | -Os |

---

### Pin Change Interrupts on AVR-Dx

The AVR-Dx family does **not** have a legacy `PCINT` peripheral.  
Instead, every GPIO pin has an **Input/Sense Configuration (ISC)** field in its `PINnCTRL` register:

| ISC setting | Trigger condition |
|-------------|-------------------|
| `PORT_ISC_INTDISABLE_gc` | Interrupt disabled (input buffer still active) |
| `PORT_ISC_BOTHEDGES_gc` | Both rising and falling edges |
| `PORT_ISC_RISING_gc` | Rising edge only |
| `PORT_ISC_FALLING_gc` | **Falling edge only ← used here** |
| `PORT_ISC_INPUT_DISABLE_gc` | Digital input buffer disabled |
| `PORT_ISC_LEVEL_gc` | Low level (continuous while asserted) |

When the configured condition is met, the corresponding bit in `PORTx.INTFLAGS` is set and the shared `PORTx_PORT_vect` ISR fires. The ISR **must clear the flag** by writing `1` to the set bit.

---

### Configuration

```c
/* PC6 – LED0 output, initially off */
PORTC.DIRSET  = PIN6_bm;
PORTC.OUTSET  = PIN6_bm;

/* PC7 – SW0 input: pull-up + falling-edge interrupt */
PORTC.DIRCLR  = PIN7_bm;
PORTC.PIN7CTRL = PORT_PULLUPEN_bm       /* Internal pull-up            */
               | PORT_ISC_FALLING_gc;   /* Wake on button press (→ LOW) */
```

No separate interrupt enable register is needed — setting ISC to anything other than `INTDISABLE` automatically arms the interrupt for that pin.

---

### How It Works

```
Power-On Reset
     │
     ▼
PORT_init()   → PC6 output (LED off), PC7 input (pull-up + falling ISC)
SLEEP_init()  → Power-Down mode selected
sei()         → Global interrupts enabled
     │
     ▼
┌──────────────────────────┐
│  sleep_cpu()             │  CPU halted, draws < 1 µA
│  (Power-Down sleep)      │  No clocks needed — ISC is asynchronous
└──────────┬───────────────┘
           │  SW0 pressed → PC7 goes LOW (falling edge)
           ▼
      ISR(PORTC_PORT_vect)
      ├─ Check PORTC.INTFLAGS & PIN7_bm
      ├─ Clear flag: PORTC.INTFLAGS = PIN7_bm
      └─ Toggle PC6 (LED)
           │
           └──────────────────────► back to sleep_cpu()
```

---

### Why Power-Down Works (no RUNSTBY needed)

Pin change detection on AVR-Dx uses the **asynchronous path** of the digital input buffer. It does not require any peripheral clock. This means the ISC trigger can wake the MCU even from the deepest **Power-Down** sleep mode, where CLK_PER and all synchronous clocks are stopped.

---

### Six-Way Comparison

| Feature | RTC PIT | RTC OVF | RTC CMP | TCB0 | ADC0 RESRDY | **Pin Change** |
|---------|---------|---------|---------|------|-------------|----------------|
| Wake source | Timer | Timer | Timer | Timer | ADC result | **Button press** |
| Periodic? | Yes (1 s) | Yes (1 s) | Yes (1 s) | Yes (1 s) | Yes (1 s) | **No (on demand)** |
| External trigger | No | No | No | No | No | **Yes (SW0)** |
| Peripherals used | RTC | RTC | RTC | TCB0+TCA0 | RTC+ADC0 | **None** |
| Sleep mode | Power-Down | Power-Down | Power-Down | Standby | Power-Down | **Power-Down** |
| Interrupt vector | `RTC_PIT_vect` | `RTC_CNT_vect` | `RTC_CNT_vect` | `TCB0_INT_vect` | `ADC0_RESRDY_vect` | **`PORTC_PORT_vect`** |
| ISR clears flag via | `RTC.PITINTFLAGS` | `RTC.INTFLAGS` | `RTC.INTFLAGS` | `TCB0.INTFLAGS` | Read `ADC0.RES` | **`PORTC.INTFLAGS`** |

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp_PinChange.atsln`** in Atmel Studio 7 / Microchip Studio 7.
3. Ensure **Microchip.AVR-Dx_DFP 2.4.286** is installed *(Tools → Device Pack Manager)*.
4. Connect the Curiosity Nano board via USB.
5. **Build → Build Solution** (F7).
6. **Debug → Start Without Debugging** (Ctrl+Alt+F5) to program and run.

---

### Expected Behaviour
- LED0 is **off** at startup; MCU is in Power-Down.
- Each press of **SW0** wakes the CPU, toggles LED0, and returns to sleep.
- Current consumption in sleep is typically **< 1 µA** (no clocks, no peripherals running).

---

### File Structure
```
LowPowerWakeUp_PinChange/
├── LowPowerWakeUp_PinChange.atsln          ← Atmel Studio solution
├── README.md
└── LowPowerWakeUp_PinChange/
    ├── LowPowerWakeUp_PinChange.cproj      ← GCC C Executable project
    └── main.c                              ← All application code
```
