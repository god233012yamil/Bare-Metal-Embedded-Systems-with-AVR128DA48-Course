# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Planned
- TX FIFO with DRE interrupt for non-blocking telemetry
- Command parser state machine for runtime threshold adjustment via USART
- Low-power idle sleep between scheduler ticks (SLPCTRL IDLE mode)
- Unit test harness (host-side, register-mock approach)

---

## [1.0.0] — 2026-03-29

### Added
- Initial production release.
- `adc.c / adc.h` — ADC0 driver: 12-bit single-ended, interrupt-driven result capture via RESRDY ISR, shared VREF configuration (2.048 V internal reference).
- `tca.c / tca.h` — TCA0 PWM driver: single-slope mode, WO0 on PD0 (via PORTMUX), glitch-free duty updates via CMP0BUF double-buffer.
- `ac.c / ac.h` — AC0 comparator driver: DACREF threshold control, BOTHEDGE interrupt, software event flag decoupled from ISR.
- `usart.c / usart.h` — USART0 driver: 115200 8N1 on PA0/PA1 (CDC-USB bridge), interrupt-driven RX with FIFO, polled TX.
- `evsys.c / evsys.h` — EVSYS channel 0 wiring: AC0_OUT generator → USERADC0START user, enabling autonomous ADC triggering on threshold crossings.
- `scheduler.c / scheduler.h` — Cooperative scheduler: RTC PIT at ~1024 Hz, up to 8 tasks, non-blocking dispatch, per-task enable/disable.
- `fifo.c / fifo.h` — Generic lock-free power-of-two ring buffer, safe for single-ISR-producer / single-main-consumer use without disabling interrupts.
- `main.c` — Application layer: 4-state control FSM (IDLE → SAMPLING → ADJUSTING → RUNNING), four scheduler tasks (ADC, Control, Telemetry, LED), CSV telemetry output.
- `inc/config.h` — Centralised compile-time configuration: clock, pin assignments, PWM parameters, baud rate, FIFO size, control thresholds.
- `PWM_Power_Module.cproj` — Atmel Studio 7 / Microchip Studio project file targeting AVR128DA48, DFP 2.4.286.

### Fixed
- `ac.c` `AC_GetOutput()`: replaced non-existent `AC_ACOUT_bm` with the correct `AC_CMPSTATE_bm` symbol from `ioavr128da48.h`.

---

[Unreleased]: https://github.com/your-username/PWM_Power_Module/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/your-username/PWM_Power_Module/releases/tag/v1.0.0
