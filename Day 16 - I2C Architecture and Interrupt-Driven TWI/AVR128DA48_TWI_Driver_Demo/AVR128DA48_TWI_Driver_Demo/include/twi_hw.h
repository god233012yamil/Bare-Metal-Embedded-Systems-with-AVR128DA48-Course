/**
 * @file twi_hw.h
 * @brief Layer 1 - TWI Hardware Control for AVR128DA48
 *
 * This header provides low-level register access, interrupt control,
 * and hardware command primitives for the TWI peripheral on the AVR-Dx
 * device family. No application code should include this file directly;
 * it is consumed only by the transaction engine (Layer 2).
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

#ifndef TWI_HW_H_
#define TWI_HW_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Clock / Baud configuration
 * FCLK = 4 MHz (default internal oscillator after reset)
 * Target I2C speed = 100 kHz (Standard Mode)
 *
 * BAUD = (FCLK / (2 * FSCL)) - 5
 *      = (4 000 000 / 200 000) - 5
 *      = 20 - 5 = 15
 * ----------------------------------------------------------------------- */
#define TWI_BAUD_VALUE      15u     /**< Baud register value for 100 kHz @ 4 MHz */

/* -----------------------------------------------------------------------
 * TWI Master Status register bit masks (MSTATUS)
 * ----------------------------------------------------------------------- */
#define TWI_M_BUSSTATE_IDLE     (0x01u << TWI_BUSSTATE_gp)  /**< Bus is idle */
#define TWI_M_BUSSTATE_OWNER    (0x03u << TWI_BUSSTATE_gp)  /**< We own the bus */
#define TWI_M_RIF               TWI_RIF_bm    /**< Read interrupt flag */
#define TWI_M_WIF               TWI_WIF_bm    /**< Write interrupt flag */
#define TWI_M_RXACK             TWI_RXACK_bm  /**< Received ACK/NACK (0=ACK) */
#define TWI_M_ARBLOST           TWI_ARBLOST_bm /**< Arbitration lost flag */
#define TWI_M_BUSERR            TWI_BUSERR_bm  /**< Bus error flag */

/* -----------------------------------------------------------------------
 * TWI Master Control register A commands (MCTRLB)
 * ----------------------------------------------------------------------- */
#define TWI_CMD_NOACT           (0x00u)  /**< No action */
#define TWI_CMD_REPSTART        (0x01u)  /**< Issue repeated START */
#define TWI_CMD_RECVTRANS       (0x02u)  /**< Receive and transmit next byte */
#define TWI_CMD_STOP            (0x03u)  /**< Issue STOP */

/* -----------------------------------------------------------------------
 * Inline hardware primitives
 * These are the only functions that touch TWI registers directly.
 * ----------------------------------------------------------------------- */

/**
 * @brief Enable the TWI master and set the SCL baud rate.
 */
static inline void twi_hw_master_enable(void)
{
    TWI0.MBAUD    = TWI_BAUD_VALUE;          /* Set SCL frequency */
    TWI0.MCTRLA   = TWI_RIEN_bm             /* Enable read interrupt */
                  | TWI_WIEN_bm             /* Enable write interrupt */
                  | TWI_ENABLE_bm;          /* Enable master */
    TWI0.MSTATUS  = TWI_BUSSTATE_IDLE_gc;   /* Force bus state to IDLE */
}

/**
 * @brief Disable the TWI master and clear all control bits.
 */
static inline void twi_hw_master_disable(void)
{
    TWI0.MCTRLA = 0x00u;   /* Clears ENABLE and all interrupt enables */
}

/**
 * @brief Issue a START condition and send the address+R/W byte.
 * @param addr_rw  7-bit address shifted left by 1, OR'd with R/W bit
 *                 (0 = write, 1 = read).
 */
static inline void twi_hw_start(uint8_t addr_rw)
{
    TWI0.MADDR = addr_rw;   /* Writing MADDR triggers START + address phase */
}

/**
 * @brief Issue a STOP condition on the bus.
 */
static inline void twi_hw_stop(void)
{
    /* Write STOP command; keep ACKACT=0 (ACK) */
    TWI0.MCTRLB = (TWI0.MCTRLB & ~TWI_MCMD_gm) | TWI_MCMD_STOP_gc;
}

/**
 * @brief Issue a REPEATED START condition.
 */
static inline void twi_hw_repeated_start(void)
{
    TWI0.MCTRLB = (TWI0.MCTRLB & ~TWI_MCMD_gm) | TWI_MCMD_REPSTART_gc;
}

/**
 * @brief Send ACK after receiving a byte (used to request another byte).
 */
static inline void twi_hw_ack(void)
{
    /* ACKACT=0 → ACK, CMD=RECVTRANS → clock next byte */
    TWI0.MCTRLB = (TWI0.MCTRLB & ~(TWI_ACKACT_bm | TWI_MCMD_gm))
                | TWI_MCMD_RECVTRANS_gc;
}

/**
 * @brief Send NACK after receiving a byte (used to signal last byte read).
 */
static inline void twi_hw_nack(void)
{
    /* ACKACT=1 → NACK, CMD=RECVTRANS → clock final byte then stop */
    TWI0.MCTRLB = (TWI0.MCTRLB & ~TWI_MCMD_gm)
                | TWI_ACKACT_bm
                | TWI_MCMD_RECVTRANS_gc;
}

/**
 * @brief Write one data byte to the master data register.
 * @param data  Byte to transmit.
 */
static inline void twi_hw_write_byte(uint8_t data)
{
    TWI0.MDATA = data;   /* Writing MDATA triggers the next transmit cycle */
}

/**
 * @brief Read the last received byte from the master data register.
 * @return Received byte value.
 */
static inline uint8_t twi_hw_read_byte(void)
{
    return TWI0.MDATA;   /* Reading MDATA returns the shifted-in byte */
}

/**
 * @brief Read the current master status register.
 * @return Raw MSTATUS byte.
 */
static inline uint8_t twi_hw_status(void)
{
    return TWI0.MSTATUS;
}

/**
 * @brief Clear bus error and arbitration-lost flags by writing 1 to them.
 */
static inline void twi_hw_clear_flags(void)
{
    TWI0.MSTATUS = TWI_ARBLOST_bm | TWI_BUSERR_bm;
}

#endif /* TWI_HW_H_ */
