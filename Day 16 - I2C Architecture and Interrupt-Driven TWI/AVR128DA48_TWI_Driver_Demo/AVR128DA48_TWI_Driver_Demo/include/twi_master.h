/**
 * @file twi_master.h
 * @brief Layer 2 - TWI Transaction Engine for AVR128DA48
 *
 * Exposes a non-blocking, interrupt-driven I2C master transaction engine.
 * The engine operates a state machine inside the TWI ISR and provides a
 * simple request/poll API to Layer 3 device drivers.
 *
 * Usage pattern:
 *   1. Call twi_master_init() once during system startup.
 *   2. Build a TWI_Transaction_t descriptor.
 *   3. Submit it with twi_master_submit().
 *   4. Poll twi_master_busy() or wait for the callback.
 *   5. Check twi_master_result() for success/error.
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

#ifndef TWI_MASTER_H_
#define TWI_MASTER_H_

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Public constants
 * ----------------------------------------------------------------------- */
#define TWI_MAX_BUFFER_LEN   64u   /**< Maximum bytes per transaction */

/* -----------------------------------------------------------------------
 * Transaction state machine states
 * ----------------------------------------------------------------------- */
typedef enum
{
    TWI_STATE_IDLE          = 0,  /**< No transaction in progress */
    TWI_STATE_START         = 1,  /**< START condition being issued */
    TWI_STATE_SEND_ADDRESS  = 2,  /**< Address + R/W byte on the wire */
    TWI_STATE_WRITE_DATA    = 3,  /**< Transmitting data bytes */
    TWI_STATE_READ_DATA     = 4,  /**< Receiving data bytes */
    TWI_STATE_REPEATED_START= 5,  /**< Repeated START for combined xfer */
    TWI_STATE_STOP          = 6,  /**< STOP condition being issued */
    TWI_STATE_COMPLETE      = 7,  /**< Transaction finished successfully */
    TWI_STATE_ERROR         = 8   /**< Transaction aborted with error */
} TWI_State_t;

/* -----------------------------------------------------------------------
 * Result / error codes returned after a transaction
 * ----------------------------------------------------------------------- */
typedef enum
{
    TWI_RESULT_OK           = 0,  /**< Success */
    TWI_RESULT_NACK_ADDR    = 1,  /**< NACK received after address byte */
    TWI_RESULT_NACK_DATA    = 2,  /**< NACK received during data phase */
    TWI_RESULT_ARB_LOST     = 3,  /**< Arbitration lost */
    TWI_RESULT_BUS_ERROR    = 4,  /**< Bus error detected */
    TWI_RESULT_TIMEOUT      = 5,  /**< Software timeout expired */
    TWI_RESULT_BUSY         = 6   /**< Engine busy, request not accepted */
} TWI_Result_t;

/* -----------------------------------------------------------------------
 * Transaction descriptor
 * Fill this structure and pass it to twi_master_submit().
 * ----------------------------------------------------------------------- */
typedef struct
{
    uint8_t      address;         /**< 7-bit slave address (not shifted) */

    uint8_t     *tx_buf;          /**< Pointer to data to transmit (may be NULL) */
    uint8_t      tx_len;          /**< Number of bytes to transmit */

    uint8_t     *rx_buf;          /**< Pointer to receive buffer (may be NULL) */
    uint8_t      rx_len;          /**< Number of bytes to receive */

    /** Optional callback invoked from ISR context when transaction ends.
     *  Set to NULL if not needed. */
    void (*callback)(TWI_Result_t result);
} TWI_Transaction_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief Initialise the TWI master hardware and engine state.
 *        Must be called once before any other TWI function.
 */
void twi_master_init(void);

/**
 * @brief Submit a transaction to the engine.
 * @param t  Pointer to a fully populated TWI_Transaction_t descriptor.
 *           The descriptor (and its buffers) must remain valid until the
 *           transaction completes.
 * @return   TWI_RESULT_OK if accepted, TWI_RESULT_BUSY if engine busy.
 */
TWI_Result_t twi_master_submit(TWI_Transaction_t *t);

/**
 * @brief Check whether a transaction is currently in progress.
 * @return true while the engine is active.
 */
bool twi_master_busy(void);

/**
 * @brief Return the result of the last completed transaction.
 * @return Result code; only valid after twi_master_busy() returns false.
 */
TWI_Result_t twi_master_result(void);

/**
 * @brief Block until the current transaction finishes (busy-wait helper).
 *        Interrupts must be enabled for this to return.
 * @return Result of the transaction.
 */
TWI_Result_t twi_master_wait(void);

#endif /* TWI_MASTER_H_ */
