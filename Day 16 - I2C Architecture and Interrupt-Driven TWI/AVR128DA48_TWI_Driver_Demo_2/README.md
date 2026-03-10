# AVR128DA48 TWI (I2C) Non-Blocking Driver Demo (Interrupt + State Machine)

Target:
- MCU: AVR128DA48
- Board: AVR128DA48 Curiosity Nano
- IDE: Microchip Studio / Atmel Studio 7 (GCC C Executable Project)
- Device Pack: AVR-Dx Device Pack (tested with 2.4.286.x)

This project implements a non-blocking TWI (I2C) master driver based on Day 16 concepts:
- Interrupt-driven transfers (byte-by-byte in ISR)
- Transaction/state machine
- Repeated START support (write register address, then read)
- Timeout protection using a 1 ms TCB tick
- Cooperative multitasking (main loop never blocks on TWI flags)

## Driver States
- IDLE
- START
- ADDR_W
- WRITE
- REP_START
- ADDR_R
- READ
- STOP
- COMPLETE
- ERROR

## Demo
The demo repeatedly performs a "register read" transaction:
1) Write register address (1 byte)
2) Repeated START
3) Read N bytes
4) STOP

Configure the target I2C device in include/board.h:
- DEMO_I2C_ADDR (7-bit)
- DEMO_I2C_REG
- DEMO_I2C_READ_LEN

If no device is connected at that address, the driver will report NACK/timeout and the LED will blink slowly.

## Notes
- SDA/SCL routing depends on PORTMUX and board wiring.
- If you need internal pull-ups, enable them in board_init_i2c_pins() (board.h).

## Files
- include/board.h
- include/twi_nb.h
- src/twi_nb.c
- src/main.c
