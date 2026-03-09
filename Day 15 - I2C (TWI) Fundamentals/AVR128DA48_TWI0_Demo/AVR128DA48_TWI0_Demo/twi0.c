#include "twi0.h"

#include <avr/io.h>

/*
File: twi0.c

Blocking TWI0 (I2C) helpers for AVR128DA48.

This module focuses on fundamentals:
- Wait for WIF/RIF before continuing
- Check RXACK after each transmitted byte (address, data, register)
- Always release the bus with STOP on error

The code is intentionally simple for course use.
*/

#ifndef F_CPU
#define F_CPU 4000000UL
#endif

/* Datasheet-style MBAUD approximation for AVR Dx TWI Host mode.
   This simple formula is fine for teaching and many real cases:

       MBAUD = (F_CPU / (2 * baud)) - 5

   Clamp to 0..255 for the 8-bit register.

   NOTE: For very accurate timing (rise time compensation, fast mode plus, etc.),
   refer to the datasheet and Microchip app notes.
*/
static uint8_t twi0_calc_mbaud(uint32_t baud_hz)
{
    uint32_t mbaud;

    /* Avoid division by zero. */
    if (baud_hz == 0u)
    {
        baud_hz = 100000u;
    }

    mbaud = (uint32_t)((F_CPU / (2u * baud_hz)) - 5u);

    if (mbaud > 255u)
    {
        mbaud = 255u;
    }

    return (uint8_t)mbaud;
}

void twi0_init(uint32_t baud_hz)
{
    /* Host mode, enable TWI0. */
    TWI0.MCTRLA = TWI_ENABLE_bm;

    /* Set baud rate. */
    TWI0.MBAUD = twi0_calc_mbaud(baud_hz);

    /* Force bus state to IDLE (recommended after enabling). */
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

bool twi0_write_byte(uint8_t address, uint8_t data)
{
    /* Send START + address (write). */
    TWI0.MADDR = (uint8_t)(address << 1);

    /* Wait for Write Interrupt Flag (WIF). */
    while (!(TWI0.MSTATUS & TWI_WIF_bm))
    {
        /* Blocking wait. */
    }

    /* Check for ACK after address. RXACK=1 means NACK received. */
    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        /* NACK -> STOP and fail. */
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* Send data byte. */
    TWI0.MDATA = data;

    /* Wait for WIF. */
    while (!(TWI0.MSTATUS & TWI_WIF_bm))
    {
        /* Blocking wait. */
    }

    /* Check for ACK after data. */
    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        /* NACK -> STOP and fail. */
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* STOP on success. */
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    return true;
}

bool twi0_read_byte(uint8_t address, uint8_t *out_data)
{
    uint8_t data;

    if (out_data == (void *)0)
    {
        return false;
    }

    /* Send START + address (read). */
    TWI0.MADDR = (uint8_t)((address << 1) | 0x01u);

    /* Wait for Read Interrupt Flag (RIF). */
    while (!(TWI0.MSTATUS & TWI_RIF_bm))
    {
        /* Blocking wait. */
    }

    /* Check for ACK after address. */
    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        /* NACK -> STOP and fail. */
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* Read data byte. */
    data = TWI0.MDATA;

    /* Single-byte read: send NACK + STOP. */
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;

    *out_data = data;
    return true;
}

bool twi0_read_reg(uint8_t dev_addr, uint8_t reg, uint8_t *out_data)
{
    uint8_t data;

    if (out_data == (void *)0)
    {
        return false;
    }

    /* --- Step 1: START + address (write) --- */
    TWI0.MADDR = (uint8_t)(dev_addr << 1);

    while (!(TWI0.MSTATUS & TWI_WIF_bm))
    {
        /* Blocking wait. */
    }

    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* --- Step 2: write register address --- */
    TWI0.MDATA = reg;

    while (!(TWI0.MSTATUS & TWI_WIF_bm))
    {
        /* Blocking wait. */
    }

    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* --- Step 3: Repeated START + address (read) --- */
    TWI0.MADDR = (uint8_t)((dev_addr << 1) | 0x01u);

    while (!(TWI0.MSTATUS & TWI_RIF_bm))
    {
        /* Blocking wait. */
    }

    if (TWI0.MSTATUS & TWI_RXACK_bm)
    {
        TWI0.MCTRLB = TWI_MCMD_STOP_gc;
        return false;
    }

    /* --- Step 4: read data --- */
    data = TWI0.MDATA;

    /* Single-byte read: NACK + STOP. */
    TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;

    *out_data = data;
    return true;
}
