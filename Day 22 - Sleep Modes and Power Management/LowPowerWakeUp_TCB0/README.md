# Low-Power Periodic Wake-Up System — TCB0 Periodic Interrupt
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates a low-power periodic wake-up using **TCB0 in Periodic Interrupt (INT) mode**.  
Unlike the RTC-based projects, TCB0 is part of the synchronous clock domain (CLK_PER), which requires **Standby** sleep mode instead of Power-Down.  TCA0 is used purely as a clock prescaler to bring the 4 MHz system clock down to a range where TCB0's 16-bit counter can achieve a clean 1-second period.

---

### Hardware
| Item | Detail |
|------|--------|
| MCU  | AVR128DA48 |
| Board | AVR128DA48 Curiosity Nano |
| LED0 | PC6 (active LOW) |
| Programmer | On-board nEDBG (UPDI) |

---

### Software
| Item | Detail |
|------|--------|
| IDE | Atmel Studio 7 / Microchip Studio 7 |
| Project type | GCC C Executable Project |
| Device Pack | Microchip.AVR-Dx_DFP 2.4.286 |
| Optimisation | -Os |

---

### Clock Architecture

```
OSCHF (4 MHz, default after reset)
        │
        ▼
   CLK_PER (4 MHz)
        │
        ├──► TCA0 (prescaler DIV64)
        │         └──► CLK_TCA = 62 500 Hz
        │                   │
        │                   ▼
        │              TCB0 (CLK_TCA source)
        │              CCMP = 62 499
        │              Period = 62 500 ticks / 62 500 Hz = 1.000 s
        │
        └──► CPU, SRAM, other peripherals (halted in Standby)
```

---

### Period Calculation

```
CLK_PER          = 4 000 000 Hz  (OSCHF default)
TCA0 prescaler   = DIV64
CLK_TCA          = 4 000 000 / 64 = 62 500 Hz

TCB0 CCMP        = (CLK_TCA × period) - 1
                 = (62 500 × 1) - 1
                 = 62 499                 ← fits in uint16_t (max 65535) ✓

Actual period    = 62 500 / 62 500 Hz = 1.000 000 s  ✓
```

Why not use TCB's own DIV2 prescaler directly from CLK_PER?
- CLK_PER / 2 = 2 000 000 Hz → CCMP = 1 999 999 → **exceeds 16-bit range**
- Using TCA0 as a pre-scaler chain solves this cleanly with an exact integer.

---

### TCB0 Periodic Interrupt Mode (CNTMODE = INT)

In this mode:
- CNT counts **0 → CCMP**
- On CNT == CCMP: CAPT interrupt fires, CNT **resets to 0 automatically**
- No manual CCMP update is needed in the ISR (unlike RTC CMP)
- The capture/compare register (CCMP) doubles as both the period register and the interrupt flag source

---

### Why Standby Instead of Power-Down?

| Sleep Mode | CLK_PER | TCB0 | RTC |
|------------|---------|------|-----|
| Power-Down | STOPPED | FROZEN ✗ | Running ✓ |
| Standby    | Running (for RUNSTDBY peripherals) | Running ✓ | Running ✓ |

TCB0 relies on the synchronous CLK_PER domain. Power-Down halts CLK_PER, which stops TCB0. **Standby** sleep keeps CLK_PER active for any peripheral with `RUNSTDBY` set — both TCA0 and TCB0 have this bit set in this project.

---

### How It Works

```
Power-On Reset
     │
     ▼
LED_init()   → PC6 as output, LED off
TCA0_init()  → DIV64 prescaler, RUNSTDBY enabled
TCB0_init()  → CCMP=62499, INT mode, CLK_TCA source, RUNSTDBY enabled
SLEEP_init() → Standby mode selected
sei()        → Global interrupts enabled
     │
     ▼
┌──────────────────────────────┐
│  sleep_cpu()                 │  CPU halted; TCA0+TCB0 still tick
│  (Standby sleep)             │
└─────────┬────────────────────┘
          │  TCB0 CNT == CCMP (every 1 second)
          ▼
     ISR(TCB0_INT_vect)
     ├─ Clear CAPT flag  (TCB0.INTFLAGS = TCB_CAPT_bm)
     └─ Toggle PC6 (LED)
          │
          └──────────────────────► back to sleep_cpu()
```

---

### Four-Way Comparison

| Feature | RTC PIT | RTC OVF | RTC CMP | TCB0 INT |
|---------|---------|---------|---------|----------|
| Peripheral | RTC | RTC | RTC | TCB0 + TCA0 |
| Clock domain | Async (OSC32K) | Async (OSC32K) | Async (OSC32K) | Sync (CLK_PER) |
| Sleep mode | Power-Down | Power-Down | Power-Down | **Standby** |
| ISR updates register? | No | No | Yes (CMP +=) | No |
| Interrupt vector | `RTC_PIT_vect` | `RTC_CNT_vect` | `RTC_CNT_vect` | `TCB0_INT_vect` |
| Period flexibility | Power-of-2 only | Any (16-bit) | Any (16-bit) | Any (16-bit, via prescaler) |
| Current in sleep | < 10 µA | < 10 µA | < 10 µA | Higher (clocks running) |

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp_TCB0.atsln`** in Atmel Studio 7 / Microchip Studio 7.
3. Ensure **Microchip.AVR-Dx_DFP 2.4.286** is installed *(Tools → Device Pack Manager)*.
4. Connect the Curiosity Nano board via USB.
5. **Build → Build Solution** (F7).
6. **Debug → Start Without Debugging** (Ctrl+Alt+F5) to program and run.

---

### Expected Behaviour
- LED0 toggles **once per second**.
- Current in Standby sleep is higher than Power-Down (system clock still active), but still significantly lower than full active mode.

---

### File Structure
```
LowPowerWakeUp_TCB0/
├── LowPowerWakeUp_TCB0.atsln          ← Atmel Studio solution
├── README.md
└── LowPowerWakeUp_TCB0/
    ├── LowPowerWakeUp_TCB0.cproj      ← GCC C Executable project
    └── main.c                         ← All application code
```
