# Low-Power Periodic Wake-Up System
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates a low-power periodic wake-up system using the AVR128DA48 microcontroller.  
The CPU is active for only a few microseconds each second — the rest of the time it sleeps in Power-Down mode.

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

### How It Works

```
Power-On Reset
     │
     ▼
LED_init()   → PC6 as output, LED off
RTC_init()   → OSC32K clock, PIT period = 32768 cycles (1 s)
SLEEP_init() → Power-Down mode selected
sei()        → Global interrupts enabled
     │
     ▼
┌────────────────────────┐
│  sleep_cpu()           │  CPU halted – draws < 10 µA
│  (Power-Down sleep)    │
└────────┬───────────────┘
         │  RTC PIT fires every 1 second
         ▼
    ISR(RTC_PIT_vect)
    ├─ Clear PI flag
    └─ Toggle PC6 (LED)
         │
         └──────────────────► back to sleep_cpu()
```

---

### RTC / PIT Configuration

| Parameter | Value |
|-----------|-------|
| Clock source | Internal 32.768 kHz oscillator (`OSC32K`) |
| PIT period register | `RTC_PERIOD_CYC32768_gc` |
| Calculated period | 32 768 / 32 768 Hz = **1.000 s** |
| Sleep mode | Power-Down (`SLPCTRL_SMODE_PDOWN_gc`) |

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp.atsln`** in Atmel Studio 7 / Microchip Studio 7.
3. Ensure the **Microchip.AVR-Dx_DFP 2.4.286** device pack is installed  
   *(Tools → Device Pack Manager)*.
4. Connect the Curiosity Nano board via USB.
5. Select **Build → Build Solution** (F7).
6. Select **Debug → Start Without Debugging** (Ctrl+Alt+F5) to program and run.

---

### Expected Behaviour
- LED0 toggles **once per second**.
- Current consumption in sleep is typically **< 10 µA** (core off, RTC running).

---

### File Structure
```
LowPowerWakeUp/
├── LowPowerWakeUp.atsln          ← Atmel Studio solution
└── LowPowerWakeUp/
    ├── LowPowerWakeUp.cproj      ← GCC C Executable project
    └── main.c                    ← All application code
```
