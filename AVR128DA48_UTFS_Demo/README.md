# AVR128DA48 UTFS Atmel Studio 7 Demo

This project demonstrates how to integrate UTFS with an AVR128DA48 MCU using the internal EEPROM as the UTFS backing store.

## Target

- MCU: AVR128DA48
- IDE: Atmel Studio 7 / Microchip Studio
- Toolchain: AVR/GNU C Compiler
- CPU/peripheral clock: 1.2 MHz (12 MHz internal oscillator divided by 10)
- Serial port: USART0
- Baud rate: 115200 8N1
- EEPROM UTFS region: 512 bytes

## What the demo stores

UTFS registers two named files:

- `config`: device name, boot count, baud rate, sample period, and debug flag
- `counter`: persistent counter value

The firmware loads the files at boot. If the EEPROM region is blank or the signatures are invalid, defaults are created and saved.

## UART commands

Open a serial terminal at 115200 baud and use these commands:

```text
help
show
inc
debug 0
debug 1
period 500
save
erase
status
```

## Source files

```text
src/main.c
src/app_storage.c
src/app_storage.h
src/storage_eeprom.c
src/storage_eeprom.h
src/uart0.c
src/uart0.h
src/utfs.c
src/utfs.h
```

## Notes

The UTFS integration point is the storage backend. For this AVR demo, the required `sys_read()` and `sys_write()` functions are implemented in `storage_eeprom.c` using avr-libc EEPROM functions.

For production firmware, consider adding CRCs, redundant copies, or an A/B commit scheme if power loss during EEPROM writes is a system-level risk.
