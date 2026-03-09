#ifndef TWI0_H
#define TWI0_H

#include <stdint.h>
#include <stdbool.h>

/*
File: twi0.h

Minimal, blocking TWI0 (I2C) helper API for AVR128DA48.

This module is designed for teaching:
- Simple host-mode setup
- Blocking transfers
- Explicit ACK checking (RXACK bit)
- Clear START / Repeated START / STOP sequencing

Notes:
- Use external pull-ups on SDA/SCL.
- Default pins for TWI0 on AVR128DA48 Curiosity Nano are PA2 (SDA) and PA3 (SCL).
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
Initialize TWI0 in Host mode.

Args:
    baud_hz: Desired I2C clock in Hz (example: 100000 for 100 kHz).

Returns:
    None.
*/
void twi0_init(uint32_t baud_hz);

/*
Write one byte to an I2C device (blocking) with ACK checking.

Sequence:
    START + address(write)
    data
    STOP

Args:
    address: 7-bit I2C address (0x00..0x7F).
    data: Byte to transmit.

Returns:
    true  - Success (device ACKed address and data).
    false - Failure (NACK detected).
*/
bool twi0_write_byte(uint8_t address, uint8_t data);

/*
Read one byte from an I2C device (blocking) with ACK checking.

Sequence:
    START + address(read)
    read data
    NACK + STOP

Args:
    address: 7-bit I2C address (0x00..0x7F).
    out_data: Pointer to receive the byte.

Returns:
    true  - Success (device ACKed address).
    false - Failure (NACK detected).
*/
bool twi0_read_byte(uint8_t address, uint8_t *out_data);

/*
Read one register from a typical I2C sensor (blocking) with ACK checking.

Sequence:
    START + address(write)
    write register
    Repeated START + address(read)
    read data
    NACK + STOP

Args:
    dev_addr: 7-bit I2C address of the device.
    reg: Register address to read.
    out_data: Pointer to receive the register value.

Returns:
    true  - Success (all ACKs received).
    false - Failure (any NACK detected).
*/
bool twi0_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *out_data);

#ifdef __cplusplus
}
#endif

#endif /* TWI0_H */
