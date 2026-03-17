# AVR128DA48 Lab: AC + EVSYS + TCB0 Capture Frequency of TCA0 WO0 PWM

This project implements:

- TCA0 generates a PWM signal (50% duty) at a known frequency and outputs it on WO0.
- AC0 detects the PWM rising edge by comparing the WO0 waveform (wired into an AC input pin)
  against a mid-supply threshold produced by DAC0.
- EVSYS routes AC0 output events (rising edges) to TCB0 capture.
- TCB0 captures the timer count on each rising edge and computes the PWM frequency.
- The measured frequency (Hz) is stored in a global variable: g_wo0_freq_hz.

Target:
- MCU: AVR128DA48
- Board: AVR128DA48 Curiosity Nano
- IDE: Microchip Studio / Atmel Studio 7 (GCC C Executable Project)
- Device Pack: AVR-Dx Device Pack 2.4.286.x (or close)

Wiring (important):
- You MUST connect the physical WO0 output pin to the physical AC0 positive input pin selected
  by AC0_POS_MUX_GC in include/board.h.

Files:
- include/board.h
- src/main.c
