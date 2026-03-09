# AVR128DA48 TWI0 (I2C) Demo - Blocking Transfers with ACK Checking

This Atmel Studio 7 / Microchip Studio project demonstrates basic TWI (I2C) host-mode transactions on the
AVR128DA48 Curiosity Nano board:

- Write one byte to a device (checks ACK)
- Read one byte from a device (checks ACK)
- Read a register from a sensor (write reg, repeated START, read data, checks ACK)

## Hardware

- Board: AVR128DA48 Curiosity Nano (DM164151)
- TWI peripheral: TWI0 in Host mode
- Default pins (no PORTMUX changes):
  - SDA: PA2
  - SCL: PA3
- User LED0 (active-low): PC6

IMPORTANT:
- You need external pull-up resistors on SDA and SCL (typ. 2.2k to 10k to VCC).
  Internal pull-ups are usually too weak for reliable I2C.

## How to Use

1. Open `AVR128DA48_TWI0_Demo.atsln` in Atmel Studio 7 / Microchip Studio.
2. Build and program the board.
3. By default, the firmware tries to read register 0x00 from a device at address 0x50.
   - If the device ACKs, LED blinks slowly.
   - If the device NACKs at any step, LED blinks fast.
4. Edit these defines in `main.c` to match your hardware:
   - `DEMO_DEV_ADDR`
   - `DEMO_REG_ADDR`

## Notes

- This code is intentionally **blocking** and designed for teaching.
- Each byte transmission checks `WIF` and `RXACK` (NACK detection).
- Each address phase in read/write also checks `RXACK` after the corresponding flag.

