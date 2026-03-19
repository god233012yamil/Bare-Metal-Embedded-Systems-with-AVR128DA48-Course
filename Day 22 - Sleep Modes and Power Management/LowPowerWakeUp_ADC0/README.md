# Low-Power Periodic Wake-Up System — ADC0 Result Ready Interrupt
## AVR128DA48 Curiosity Nano

### Overview
This project demonstrates a low-power periodic wake-up using the **ADC0 RESRDY (Result Ready) interrupt** as the wake-up source.

Because ADC0 cannot time itself, a trigger source is required to start each conversion while the CPU sleeps. The **RTC PIT** (1-second period) is routed through the **Event System (EVSYS)** to auto-trigger ADC0. When the conversion completes, the **RESRDY interrupt** wakes the CPU, the LED toggles, and the CPU returns to Power-Down sleep — all without any software-initiated ADC start.

No external wiring is needed — the on-board **internal temperature sensor** is used as the ADC input.

---

### Hardware
| Item | Detail |
|------|--------|
| MCU  | AVR128DA48 |
| Board | AVR128DA48 Curiosity Nano |
| LED0 | PC6 (active LOW) |
| ADC Input | Internal temperature sensor (MUXPOS = TEMPSENSE) |
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

### Signal Chain

```
OSC32K (32.768 kHz)
     │
     ▼
RTC PIT (CYC32768 = 1 s period)
     │  event pulse every 1 second
     ▼
EVSYS Channel 0
     │  CHANNEL0 = RTC_PIT_DIV32768
     ▼
ADC0 START trigger  (USERADC0START = CHANNEL_0)
     │  STARTEI = 1 → one conversion per event
     ▼
ADC0 converts internal temperature sensor
     │  conversion complete
     ▼
ADC0 RESRDY interrupt  ──►  wakes CPU from Power-Down
     │
     ▼
ISR(ADC0_RESRDY_vect)
     ├─ Read ADC0.RES  (clears RESRDY flag)
     └─ Toggle PC6 (LED)
          │
          └──► sleep_cpu()  (back to Power-Down)
```

---

### Key Configuration Details

#### RTC PIT
| Register | Value | Purpose |
|----------|-------|---------|
| `RTC.CLKSEL` | `OSC32K_gc` | 32.768 kHz internal oscillator |
| `RTC.PITCTRLA` | `CYC32768 \| PITEN` | 1-second event pulse; **no CPU interrupt** |

> The PIT CPU interrupt is **not enabled**. The PIT is used only as an EVSYS event generator.

#### Event System (EVSYS)
| Register | Value | Purpose |
|----------|-------|---------|
| `EVSYS.CHANNEL0` | `RTC_PIT_DIV32768_gc` | RTC PIT 1 Hz pulse on CH0 |
| `EVSYS.USERADC0START` | `CHANNEL_0_gc` | CH0 event → ADC0 start trigger |

#### ADC0
| Register | Field | Value | Purpose |
|----------|-------|-------|---------|
| `ADC0.MUXPOS` | — | `TEMPSENSE_gc` | Internal temperature sensor |
| `ADC0.CTRLC` | `REFSEL` | `1024MV_gc` | 1.024 V internal reference |
| `ADC0.CTRLB` | `PRESC` | `DIV256_gc` | ADC clock ≈ 15.6 kHz |
| `ADC0.CTRLA` | `ENABLE` | 1 | Enable ADC |
| `ADC0.CTRLA` | `RUNSTDBY` | 1 | Keep ADC running during sleep |
| `ADC0.EVCTRL` | `STARTEI` | 1 | Event-triggered start (one shot) |
| `ADC0.INTCTRL` | `RESRDY` | 1 | Interrupt when result is ready |

#### Sleep
| Register | Value | Purpose |
|----------|-------|---------|
| `SLPCTRL.CTRLA` | `PDOWN \| SEN` | Power-Down; CPU halted, RTC keeps running |

---

### Why Power-Down Works Here (vs. TCB0 which needed Standby)

| Peripheral | Clock Domain | Survives Power-Down? |
|------------|-------------|----------------------|
| RTC PIT | Async (OSC32K) | ✓ Yes |
| EVSYS | Async-capable | ✓ Yes (event routing active) |
| ADC0 (RUNSTDBY=1) | Can run from async path | ✓ Yes |
| TCB0 | Sync (CLK_PER) | ✗ No |

The RTC PIT fires its event pulse in the asynchronous domain. EVSYS forwards it to ADC0, which has `RUNSTDBY` set so it can wake from Power-Down, complete a conversion, and raise RESRDY — all without CLK_PER running.

---

### Five-Way Comparison

| Feature | RTC PIT | RTC OVF | RTC CMP | TCB0 INT | ADC0 RESRDY |
|---------|---------|---------|---------|----------|-------------|
| Wake source | RTC PIT | RTC OVF | RTC CMP | TCB0 CAPT | ADC0 RESRDY |
| Trigger mechanism | Direct ISR | Direct ISR | Direct ISR | Direct ISR | RTC PIT → EVSYS → ADC0 |
| Uses EVSYS | No | No | No | No | **Yes** |
| Peripherals used | RTC | RTC | RTC | TCB0 + TCA0 | RTC + EVSYS + ADC0 |
| Sleep mode | Power-Down | Power-Down | Power-Down | Standby | Power-Down |
| ISR updates register? | No | No | Yes (CMP) | No | No |
| Side benefit | — | — | Timestamps | — | **ADC result available** |
| Interrupt vector | `RTC_PIT_vect` | `RTC_CNT_vect` | `RTC_CNT_vect` | `TCB0_INT_vect` | `ADC0_RESRDY_vect` |

---

### Opening the Project

1. Extract the ZIP archive.
2. Open **`LowPowerWakeUp_ADC0.atsln`** in Atmel Studio 7 / Microchip Studio 7.
3. Ensure **Microchip.AVR-Dx_DFP 2.4.286** is installed *(Tools → Device Pack Manager)*.
4. Connect the Curiosity Nano board via USB.
5. **Build → Build Solution** (F7).
6. **Debug → Start Without Debugging** (Ctrl+Alt+F5) to program and run.

---

### Expected Behaviour
- LED0 toggles **once per second**, triggered by ADC0 completing a conversion of the internal temperature sensor.
- The CPU is active only for the brief ISR duration; the rest of the time it is in Power-Down.

---

### File Structure
```
LowPowerWakeUp_ADC0/
├── LowPowerWakeUp_ADC0.atsln          ← Atmel Studio solution
├── README.md
└── LowPowerWakeUp_ADC0/
    ├── LowPowerWakeUp_ADC0.cproj      ← GCC C Executable project
    └── main.c                         ← All application code
```
