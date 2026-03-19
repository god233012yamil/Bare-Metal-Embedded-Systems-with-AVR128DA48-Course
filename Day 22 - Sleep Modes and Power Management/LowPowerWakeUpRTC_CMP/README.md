# Low-Power Periodic Wake-Up System — RTC CMP Compare Match Interrupt
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates a low-power periodic wake-up using the **RTC Compare Match (CMP) interrupt**.  
Unlike the OVF approach (where CNT resets on overflow), the CMP interrupt fires when `CNT == CMP` while the counter keeps running freely. The ISR advances `CMP` by one step each time to schedule the next wake-up.

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

### Three-Way Comparison: PIT vs OVF vs CMP

| Feature | PIT | OVF | CMP |
|---------|-----|-----|-----|
| Interrupt vector | `RTC_PIT_vect` | `RTC_CNT_vect` | `RTC_CNT_vect` |
| Period control | `RTC.PITCTRLA` enum | `RTC.PER` value | `RTC.CMP` stepped in ISR |
| Counter resets on event? | N/A (no counter) | Yes (CNT → 0) | **No** (CNT runs freely) |
| Flag to clear | `RTC.PITINTFLAGS = RTC_PI_bm` | `RTC.INTFLAGS = RTC_OVF_bm` | `RTC.INTFLAGS = RTC_CMP_bm` |
| ISR must update register? | No | No | **Yes** — advance CMP |
| Simultaneous OVF + CMP? | No | N/A | Yes (both can be enabled) |
| Flexible period | No (power-of-2 only) | Yes | Yes |
| Absolute timestamp possible? | No | No | **Yes** (CMP is absolute) |

---

### Period Calculation

```
RTC clock source : OSC32K = 32 768 Hz
Prescaler        : DIV1   (no prescaling)
PER              : 0xFFFF  (free-running, full 16-bit range)

CMP step = 32 768 ticks  →  32 768 / 32 768 Hz = 1 second

Timeline:
  CNT  :  0 ──────────────────► 32768 ──────────────────► 65536(=0) ──► ...
  CMP  :                        32768        ISR sets→    65536(=0)  ──► 32768
  Event:                          ▲                          ▲
                               1st match                  2nd match
```

16-bit wrap of `CMP` is intentional — `uint16_t` addition handles it naturally.

---

### How It Works

```
Power-On Reset
     │
     ▼
LED_init()   → PC6 as output, LED off
RTC_init()   → OSC32K, PER=0xFFFF, CMP=32768,
               CMP interrupt enabled, RTC enabled DIV1 + RUNSTDBY
SLEEP_init() → Power-Down mode selected
sei()        → Global interrupts enabled
     │
     ▼
┌──────────────────────────────┐
│  sleep_cpu()                 │  CPU halted – draws < 10 µA
│  (Power-Down sleep)          │
└─────────┬────────────────────┘
          │  RTC CNT == CMP  (every 1 second)
          ▼
     ISR(RTC_CNT_vect)
     ├─ Clear CMP flag  (RTC.INTFLAGS = RTC_CMP_bm)
     ├─ Advance CMP     (RTC.CMP += 32768)
     └─ Toggle PC6 (LED)
          │
          └──────────────────────► back to sleep_cpu()
```

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp_CMP.atsln`** in Atmel Studio 7 / Microchip Studio 7.
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
LowPowerWakeUp_CMP/
├── LowPowerWakeUp_CMP.atsln          ← Atmel Studio solution
├── README.md
└── LowPowerWakeUp_CMP/
    ├── LowPowerWakeUp_CMP.cproj      ← GCC C Executable project
    └── main.c                        ← All application code
```
