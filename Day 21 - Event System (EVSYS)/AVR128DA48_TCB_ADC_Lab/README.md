# AVR128DA48 – TCB0 → EVSYS → ADC0 Lab

## Overview

Demonstrates hardware-triggered ADC conversions on the **AVR128DA48 Curiosity Nano**.

```
TCB0 (100 Hz)  -->  EVSYS Channel 0  -->  ADC0 START
                                              |
                                         RESRDY ISR
                                              |
                                         main() reads result
```

`ADC0.COMMAND = ADC_STCONV_bm` is **never** written. The CPU only reads results.

---

## Hardware

| Signal | Pin  | Notes |
|--------|------|-------|
| ADC in | PD2 (AIN2) | Connect 0–2 V (pot to VDD/GND works) |
| LED0   | PC6 (active-low) | Toggles at 100 Hz to confirm operation |

---

## Key Configuration

### TCB0 – Periodic Interrupt mode (100 Hz)
```c
TCB0.CCMP  = 0x9C3F;               // (39999+1) / 4 MHz = 10 ms
TCB0.CTRLB = TCB_CNTMODE_INT_gc;
TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;
```

### EVSYS
```c
EVSYS.CHANNEL0      = EVSYS_CHANNEL0_TCB0_CAPT_gc;
EVSYS.USERADC0START = EVSYS_USER_ADC0START_CHANNEL0_gc;
```

### ADC0 – event-triggered, 12-bit
```c
VREF.ADC0REF = VREF_REFSEL_2V048_gc;
ADC0.CTRLA   = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc | ADC_STARTEI_bm;
//                                                    ^^^^^^^^^^^^^^^
//                              This bit enables hardware-triggered starts.
```

---

## Toolchain

- **IDE**: Atmel Studio 7 / Microchip Studio  
- **Device Pack**: AVR-Dx 2.4.286  
- **Target**: AVR128DA48 (Curiosity Nano)  
- **Project type**: GCC C Executable Project  
- **Optimization**: Release → -Os / Debug → -O1 -g3  

---

## Changing the sample rate

Edit `TCB0_PERIOD` in `main.c`:

```
Period (s) = (TCB0_PERIOD + 1) / F_CPU
```

| TCB0_PERIOD | Sample rate |
|-------------|------------|
| 0x9C3F (39999) | 100 Hz |
| 0x4E1F (19999) | 200 Hz |
| 0x270F (9999)  | 400 Hz |
| 0xF9FF (63999) |  62.5 Hz |
