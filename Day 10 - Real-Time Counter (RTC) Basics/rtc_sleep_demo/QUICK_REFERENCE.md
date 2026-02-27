# AVR128DA48 RTC Sleep Wake-up - Quick Reference Guide

## Timing Diagram

```
Time (seconds):  0    1    2    3    4    5    6    7    8
                 │    │    │    │    │    │    │    │    │
Current Draw:    │    │    │    │    │    │    │    │    │
                 │    │    │    │    │    │    │    │    │
    ~4 mA ─┐    ││   ││   ││   ││   ││   ││   ││   ││   ││
           │    ││   ││   ││   ││   ││   ││   ││   ││   ││
           ├────┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───
  ~10 µA ──┘    Sleep Sleep Sleep Sleep Sleep Sleep Sleep Sleep
                ^     ^    ^    ^    ^    ^    ^    ^    ^
                │     │    │    │    │    │    │    │    │
          Wake  RTC   RTC  RTC  RTC  RTC  RTC  RTC  RTC  RTC
          spikes OVF  OVF  OVF  OVF  OVF  OVF  OVF  OVF  OVF
          (~1ms) ISR  ISR  ISR  ISR  ISR  ISR  ISR  ISR  ISR
                LED   LED  LED  LED  LED  LED  LED  LED  LED
              Toggle
```

### LED State Over Time

```
LED State:  ON  OFF ON  OFF ON  OFF ON  OFF ON  OFF ON  OFF
            ██──┐  ██──┐  ██──┐  ██──┐  ██──┐  ██──┐
               └──┘   └──┘   └──┘   └──┘   └──┘   └──┘
            ^1s ^1s ^1s ^1s ^1s ^1s ^1s ^1s ^1s ^1s ^1s
```

## RTC Counter Operation

```
32.768 kHz Oscillator → ÷32 Prescaler → 1024 Hz Counter
                                              │
                                              ↓
                                        Count: 0...1023
                                              │
                                              ↓ (When CNT == PER)
                                        Overflow Interrupt
                                              │
                                              ↓
                                        Wake MCU from Sleep
```

## Register Configuration Sequence

### 1. Clock Setup

```
CLKCTRL.OSC32KCTRLA = RUNSTDBY
     │
     ├─→ Enables 32.768 kHz oscillator
     └─→ Keeps it running in all modes
```

### 2. RTC Configuration

```
RTC.CTRLA = 0  ──→ Disable RTC (for safe configuration)
     │
     ↓
RTC.CLKSEL = OSC32K  ──→ Select internal 32.768 kHz from OSC32K
     │
     ↓
RTC.PER = 1023  ──→ Set period (1024 ticks = 1 second)
     │
     ↓
RTC.CNT = 0  ──→ Clear counter
     │
     ↓
RTC.INTCTRL = OVF_bm  ──→ Enable overflow interrupt
     │
     ↓
RTC.CTRLA = PRESCALER_DIV32 | RTCEN | RUNSTDBY
     │       │                │        │
     │       │                │        └─→ Run in standby/power-down
     │       │                └─→ Enable RTC
     │       └─→ Divide by 32 (1024 Hz)
     │
     ↓
Wait for PERBUSY flag to clear
```

### 3. Sleep Configuration

```
SLPCTRL.CTRLA = SMODE_PDOWN | SEN
                │             │
                │             └─→ Sleep Enable
                └─→ Power-Down mode
```

### 4. Main Loop Execution

```
┌─────────────────────────────────────────┐
│  while(1)                               │
│  {                                      │
│      sleep_cpu(); ──────┐               │
│                         │               │
│  } // Loop              │               │
└─────────────────────────┼───────────────┘
                          │
                          ↓
              ┌───────────────────────┐
              │  Enter Power-Down     │
              │  - CPU OFF            │
              │  - Main Clock OFF     │
              │  - RTC Running        │
              │  - Current: ~10 µA    │
              └───────────┬───────────┘
                          │
                          ↓ (After 1 second)
              ┌───────────────────────┐
              │  RTC Overflow         │
              └───────────┬───────────┘
                          │
                          ↓
              ┌───────────────────────┐
              │  Wake-up              │
              │  - Restore clocks     │
              │  - Jump to ISR        │
              └───────────┬───────────┘
                          │
                          ↓
              ┌───────────────────────┐
              │  ISR(RTC_CNT_vect)    │
              │  {                    │
              │    Clear INTFLAGS     │
              │    Toggle LED         │
              │  }                    │
              └───────────┬───────────┘
                          │
                          ↓
              ┌───────────────────────┐
              │  Return from ISR      │
              │  Continue in main()   │
              └───────────┬───────────┘
                          │
                          ↓
                     Back to while(1) loop
```

## Memory Layout

```
Flash Memory (128 KB):
┌─────────────────────────┐ 0x0000
│  Interrupt Vectors      │
├─────────────────────────┤ 0x0100
│  Program Code           │
│  - main()               │
│  - clock_init()         │
│  - gpio_init()          │
│  - rtc_init()           │
│  - sleep_init()         │
│  - ISR(RTC_CNT_vect)    │
├─────────────────────────┤
│  Constant Data          │
│  (strings, etc.)        │
├─────────────────────────┤
│  Unused Flash           │
│                         │
└─────────────────────────┘ 0x1FFFF

SRAM (16 KB):
┌─────────────────────────┐ 0x3000
│  Stack (grows down)     │
│         ↓               │
├─────────────────────────┤
│                         │
│  Unused                 │
│                         │
├─────────────────────────┤
│         ↑               │
│  Heap (grows up)        │
├─────────────────────────┤
│  Global Variables       │
│  (initialized .data)    │
├─────────────────────────┤
│  Uninitialized (.bss)   │
└─────────────────────────┘ 0x6FFF
```

## Peripheral Power Consumption

```
Peripheral State During Power-Down:

╔══════════════════╦═══════════╦═══════════╗
║   Peripheral     ║   State   ║  Current  ║
╠══════════════════╬═══════════╬═══════════╣
║ CPU              ║    OFF    ║    0 µA   ║
║ Main Clock       ║    OFF    ║    0 µA   ║
║ Peripherals      ║    OFF    ║    0 µA   ║
║ RTC (RUNSTDBY=1) ║    ON     ║  ~3 µA    ║
║ WDT (if enabled) ║    ON     ║  ~2 µA    ║
║ BOD (if enabled) ║    ON     ║  ~5 µA    ║
║ Leakage          ║    N/A    ║  ~2 µA    ║
╠══════════════════╬═══════════╬═══════════╣
║ TOTAL (typical)  ║           ║ ~10 µA    ║
╚══════════════════╩═══════════╩═══════════╝
```

## RTC Prescaler Selection Guide

Choose your prescaler based on desired wake-up frequency:

```
Wake-up Need          │ Prescaler │ PER Value │ Resolution
──────────────────────┼───────────┼───────────┼────────────
Sub-millisecond       │ DIV1      │ ~30       │ 30.5 µs
Millisecond precision │ DIV32     │ ~1000     │ 977 µs
Second precision      │ DIV32     │ 1024      │ 977 µs
Multi-second          │ DIV1024   │ ~1000     │ 31.25 ms
Minute intervals      │ DIV32768  │ 60        │ 1 second
Hour intervals        │ DIV32768  │ 3600      │ 1 second
```

## Common Configuration Examples

### Example 1: 100ms Wake-up

```c
RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
RTC.PER = 101;  // 102 ticks @ 1024 Hz ≈ 99.6 ms
RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;
```

### Example 2: 5-second Wake-up

```c
RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
RTC.PER = 5119;  // 5120 ticks @ 1024 Hz = 5 seconds
RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;
```

### Example 3: 1-minute Wake-up

```c
RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
RTC.PER = 59;  // 60 ticks @ 1 Hz = 60 seconds
RTC.CTRLA = RTC_PRESCALER_DIV32768_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;
```

## Pin State During Sleep

```
Pin Configuration Impact on Sleep Current:

Input - Floating         ║ ⚠️  HIGH CURRENT (undefined)
Input - Pull-up          ║ ✓  ~1 µA per pin
Input - Pull-down        ║ ✓  ~1 µA per pin
Output - Driving High    ║ ✓  Minimal (no load)
Output - Driving Low     ║ ✓  Minimal (no load)
Output - Floating        ║ ⚠️  Avoid this state
Analog Input             ║ ✓  Disable digital buffer
```

**Best Practice:** Configure all unused pins as inputs with pull-ups enabled.

## Debugging Checklist

```
□ OSC32K enabled with RUNSTDBY bit set
□ RTC CLKSEL configured correctly
□ RTC PER value calculated properly
□ RTC RUNSTDBY bit set in CTRLA
□ RTC interrupt enabled (INTCTRL)
□ Global interrupts enabled (sei())
□ Sleep mode configured (SLPCTRL.CTRLA)
□ LED pin configured as output
□ ISR clears interrupt flag
□ Wait for RTC busy flags after configuration
```

## Power Optimization Checklist

```
□ Disable unused peripherals (ADC, DAC, AC, USART, etc.)
□ Configure all unused pins (input with pull-up)
□ Minimize LED on-time (brief toggles only)
□ Use Power-Down mode (not Standby or Idle)
□ Disable BOD if not required (fuse setting)
□ Use lowest acceptable RTC prescaler
□ Minimize ISR execution time
□ Avoid floating pins
□ Turn off pull-ups on high-impedance inputs
□ Use external 32 kHz crystal for better accuracy (optional)
```

## Calculation Examples

### Calculate PER for specific wake-up time:

```
Given:
  - 32.768 kHz oscillator
  - DIV32 prescaler → 1024 Hz
  - Desired wake-up: 2.5 seconds

Calculation:
  PER = (2.5 seconds × 1024 Hz) - 1
  PER = 2560 - 1
  PER = 2559
```

### Calculate actual wake-up time from PER:

```
Given:
  - PER = 1500
  - Prescaler: DIV32 (1024 Hz)

Calculation:
  Time = (PER + 1) / Frequency
  Time = 1501 / 1024 Hz
  Time = 1.466 seconds
```

### Calculate average current draw:

```
Given:
  - Sleep current: 10 µA
  - Wake current: 4 mA
  - Wake duration: 1 ms
  - Wake period: 1 second

Calculation:
  Avg = (Sleep_I × Sleep_Time + Wake_I × Wake_Time) / Total_Time
  Avg = (10 µA × 999 ms + 4000 µA × 1 ms) / 1000 ms
  Avg = (9990 + 4000) / 1000
  Avg = 14 µA (approximately)
```

## Common Issues and Solutions

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| LED not blinking | RTC not configured | Check OSC32K and RTC setup |
| Blinks too fast | Wrong prescaler | Adjust RTC.CTRLA prescaler |
| Blinks too slow | Wrong PER value | Recalculate PER |
| High current draw | Floating pins | Enable pull-ups on inputs |
| No sleep mode | SEN bit not set | Check SLPCTRL.CTRLA |
| RTC stops in sleep | RUNSTDBY not set | Set RTC_RUNSTDBY_bm |
| Debugger interferes | UPDI keeps CPU on | Disconnect for accurate test |

## Further Reading

- Section 17: RTC - Real-Time Counter (Datasheet pg. 195)
- Section 18: SLPCTRL - Sleep Controller (Datasheet pg. 211)
- Section 10: CLKCTRL - Clock Controller (Datasheet pg. 133)
- Section 13: PORTMUX - Port Multiplexer (Datasheet pg. 155)
