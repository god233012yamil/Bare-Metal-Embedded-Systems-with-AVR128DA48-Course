# AVR128DA48 Non-Blocking SPI0 Driver Demo (State Machine + ISR)

This project provides a non-blocking SPI0 master driver for AVR128DA48 using:
- State machine: IDLE, ASSERT_CS, TRANSFER, COMPLETE, ERROR
- ISR-driven byte-by-byte transfer (SPI interrupt)
- Cooperative multitasking: main continues running while SPI transfer progresses
- Optional timeout support using a 1 ms TCB tick

## Hardware
- MCU: AVR128DA48
- Board: AVR128DA48 Curiosity Nano (or equivalent)

## Default Pin Mapping (edit include/board.h if needed)
SPI0:
- MOSI: PA4
- MISO: PA5
- SCK : PA6

Chip Selects (manual GPIO):
- DEV0_CS: PA7
- DEV1_CS: PB0

LED:
- LED_PIN: PA2

## Quick Loopback Test (no external device required)
Wire MOSI (PA4) to MISO (PA5).
The demo runs:
- A short pattern transfer
- A long transfer (2048 bytes) to prove non-blocking behavior

## Notes
SPI interrupt vector name and INTCTRL bit names can vary slightly with header packs.
This project targets Microchip AVR DA headers (avr/io.h). If your headers differ, adjust:
- ISR name (SPI0_INT_vect)
- SPI interrupt enable bit (SPI_IE_bm)

## Files
- include/board.h       Board mapping + device table
- include/spi_nb.h      Non-blocking driver API
- src/spi_nb.c          Driver implementation (state machine + ISR)
- src/main.c            Demo (loopback + long transfer + LED heartbeat)
