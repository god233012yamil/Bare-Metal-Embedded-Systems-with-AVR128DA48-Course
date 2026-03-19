# Low-Power Periodic Wake-Up System — RTC OVF Interrupt
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates a low-power periodic wake-up using the **RTC Overflow (OVF) interrupt** instead of the Periodic Interrupt Timer (PIT).  
The RTC counter increments from 0 to PER (32767), overflows, resets to 0, and fires the OVF interrupt — once per second.

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

### OVF vs PIT — Key Differences

| Feature | PIT (previous project) | OVF (this project) |
|---------|------------------------|---------------------|
| Interrupt vector | `RTC_PIT_vect` | `RTC_CNT_vect` |
| Period set via | `RTC.PITCTRLA` enum | `RTC.PER` register value |
| Counter register | None (PIT is separate) | `RTC.CNT` (16-bit, readable/writable) |
| Flag to clear | `RTC.PITINTFLAGS = RTC_PI_bm` | `RTC.INTFLAGS = RTC_OVF_bm` |
| Flexibility | Fixed power-of-2 periods only | Any period from 1 to 65535 ticks |
| Compare match | Not available | `RTC.CMP` available for extra event |

---

### Period Calculation

```
RTC clock source : OSC32K = 32 768 Hz
Prescaler        : DIV1   (no prescaling)
Desired period   : 1 second

PER = (RTC_clock × desired_period) - 1
    = (32 768 × 1) - 1
    = 32 767  (0x7FFF)

OVF fires when CNT transitions from 32767 → 0  (32768 ticks = 1 s)
```

---

### How It Works

```
Power-On Reset
     │
     ▼
LED_init()   → PC6 as output, LED off
RTC_init()   → OSC32K clock, PER = 32767, OVF interrupt enabled,
               RTC enabled with DIV1 prescaler + RUNSTDBY
SLEEP_init() → Power-Down mode selected
sei()        → Global interrupts enabled
     │
     ▼
┌────────────────────────────┐
│  sleep_cpu()               │  CPU halted – draws < 10 µA
│  (Power-Down sleep)        │
└────────┬───────────────────┘
         │  RTC CNT overflows (32768 ticks = 1 s)
         ▼
    ISR(RTC_CNT_vect)
    ├─ Clear OVF flag  (RTC.INTFLAGS = RTC_OVF_bm)
    └─ Toggle PC6 (LED)
         │
         └──────────────────► back to sleep_cpu()
```

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp_OVF.atsln`** in Atmel Studio 7 / Microchip Studio 7.
3. Ensure **Microchip.AVR-Dx_DFP 2.4.286** is installed *(Tools → Device Pack Manager)*.
4. Connect the Curiosity Nano board via USB.
5. **Build → Build Solution** (F7).
6. **Debug → Start Without Debugging** (Ctrl+Alt+F5) to program and run.

---

### Expected Behaviour
- LED0 toggles **once per second**.
- Current consumption in sleep is typically **< 10 µA** (CPU off, RTC running).

---

### File Structure
```
LowPowerWakeUp_OVF/
├── LowPowerWakeUp_OVF.atsln          ← Atmel Studio solution
├── README.md
└── LowPowerWakeUp_OVF/
    ├── LowPowerWakeUp_OVF.cproj      ← GCC C Executable project
    └── main.c                        ← All application code
```
