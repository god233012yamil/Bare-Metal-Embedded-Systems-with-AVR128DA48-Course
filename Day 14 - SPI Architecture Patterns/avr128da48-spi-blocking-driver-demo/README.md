# AVR128DA48 SPI0 Driver Demo (Bare Metal)

This project contains:
- A reusable SPI0 master driver with:
  - Transaction API (CS asserted for full transfer)
  - Per-device configuration (mode, prescaler, CS pin)
  - Timeout protection (prevents infinite blocking)
  - Status/error codes
- A minimal demo app showing how to use it

## Hardware
- MCU: AVR128DA48
- Board: AVR128DA48 Curiosity Nano (or equivalent AVR128DA48 setup)

## Default Pin Mapping (edit include/board.h if needed)
SPI0:
- MOSI: PA4
- MISO: PA5
- SCK : PA6

Chip Selects (manual GPIO):
- DEV0_CS: PA7
- DEV1_CS: PB0

LED:
- LED_PIN: PA2 (demo LED)

## Quick Test Option (Loopback)
To validate the driver without any external SPI device:
- Wire MOSI (PA4) to MISO (PA5)
- Leave SCK and CS connected to your scope/logic analyzer if desired
The demo sends a pattern on DEV0 and expects to read the same bytes back.

## Build Notes
This is a plain avr-gcc project layout. You can:
- Import the sources into Microchip Studio / Atmel Studio as a GCC C project, or
- Use your preferred avr-gcc build system.

If you want a Makefile-based build, add your local toolchain paths and device flags.
Typical flags:
- -mmcu=avr128da48
- -DF_CPU=24000000UL

## Files
- include/board.h        Board/pin mapping and device config table
- include/spi_driver.h   Public driver API
- src/spi_driver.c       Driver implementation
- src/main.c             Demo application

## Demo Behavior
- Initializes SPI0 and GPIO
- Runs a loopback test on DEV0 (requires MOSI<->MISO wire)
  - Toggles LED fast on success
  - Toggles LED slow on failure
- Performs a "JEDEC ID read" transaction (0x9F) on DEV0 as an example
  - If you have an SPI flash connected, you can observe meaningful bytes on MISO

