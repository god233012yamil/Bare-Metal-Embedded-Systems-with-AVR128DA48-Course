/**
 * @file twi_master.c
 * @brief Layer 2 - TWI Transaction Engine Implementation for AVR128DA48
 *
 * Implements the I2C master state machine driven entirely by the TWI0
 * master interrupt (TWI0_TWIM_vect).  All register access is delegated
 * to the Layer 1 inline primitives in twi_hw.h.
 *
 * State transition summary
 * ========================
 *   IDLE  ──submit()──►  START  ──ISR──►  SEND_ADDRESS
 *     ◄──────────────────────────────────────────────────────┐
 *   SEND_ADDRESS ─write─► WRITE_DATA ──last byte──► STOP ──► COMPLETE
 *   SEND_ADDRESS ─read──► READ_DATA  ──last byte──► STOP ──► COMPLETE
 *   SEND_ADDRESS ─w+r──►  WRITE_DATA ──►  REPEATED_START ──► READ_DATA
 *   Any state ──error──►  ERROR ──► (STOP) ──► IDLE
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

#include "twi_master.h"
#include "twi_hw.h"

#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * Internal engine context (all fields modified in ISR, read in main)
 * ----------------------------------------------------------------------- */
static volatile TWI_State_t   s_state;      /**< Current state machine state */
static volatile TWI_Result_t  s_result;     /**< Result of last transaction */

static TWI_Transaction_t     *s_current;    /**< Active transaction descriptor */
static uint8_t                s_tx_idx;     /**< Next byte index in tx_buf */
static uint8_t                s_rx_idx;     /**< Next byte index in rx_buf */

/* -----------------------------------------------------------------------
 * Private helpers (called from ISR only)
 * ----------------------------------------------------------------------- */

/**
 * @brief Terminate the current transaction with a given result.
 *        Issues a STOP, advances state, and fires the callback if set.
 * @param result  Final result code to store.
 */
static void finish_transaction(TWI_Result_t result)
{
    twi_hw_stop();                       /* Release the bus */
    s_result = result;                   /* Record outcome */
    s_state  = (result == TWI_RESULT_OK) /* Advance to terminal state */
               ? TWI_STATE_COMPLETE
               : TWI_STATE_ERROR;

    /* Invoke optional user callback from ISR context */
    if (s_current && s_current->callback)
    {
        s_current->callback(result);
    }

    s_state   = TWI_STATE_IDLE;          /* Return engine to idle */
    s_current = NULL;
}

/**
 * @brief Handle the write-data phase: transmit the next byte or finish.
 */
static void handle_write_phase(void)
{
    if (s_tx_idx < s_current->tx_len)
    {
        /* More bytes to send – write next byte to MDATA */
        twi_hw_write_byte(s_current->tx_buf[s_tx_idx++]);
    }
    else if (s_current->rx_len > 0u)
    {
        /* Tx done, but we also need to read – issue a repeated START */
        s_state = TWI_STATE_REPEATED_START;
        twi_hw_repeated_start();
        /* After repeated START the ISR re-enters via write-interrupt with
         * the address byte acknowledged; we then switch to read addressing. */
        twi_hw_start((uint8_t)((s_current->address << 1u) | 0x01u)); /* READ */
    }
    else
    {
        /* Write-only transaction complete */
        finish_transaction(TWI_RESULT_OK);
    }
}

/**
 * @brief Handle the read-data phase: ACK/NACK and store received byte.
 */
static void handle_read_phase(void)
{
    /* Store the byte that was just shifted in */
    if (s_rx_idx < s_current->rx_len)
    {
        s_current->rx_buf[s_rx_idx++] = twi_hw_read_byte();
    }

    if (s_rx_idx < s_current->rx_len)
    {
        /* More bytes expected – send ACK to clock in next byte */
        twi_hw_ack();
    }
    else
    {
        /* Last byte received – send NACK then STOP */
        twi_hw_nack();
        finish_transaction(TWI_RESULT_OK);
    }
}

/* -----------------------------------------------------------------------
 * TWI0 Master Interrupt Service Routine
 * Both read-complete (RIF) and write-complete (WIF) flags share this
 * single vector on AVR-Dx devices.
 * ----------------------------------------------------------------------- */
ISR(TWI0_TWIM_vect)
{
    uint8_t status = twi_hw_status();   /* Snapshot MSTATUS once */

    /* ---- Error conditions have highest priority ---- */
    if (status & TWI_M_BUSERR)
    {
        twi_hw_clear_flags();
        finish_transaction(TWI_RESULT_BUS_ERROR);
        return;
    }

    if (status & TWI_M_ARBLOST)
    {
        twi_hw_clear_flags();
        finish_transaction(TWI_RESULT_ARB_LOST);
        return;
    }

    /* ---- Write Interrupt Flag – address or data byte sent ---- */
    if (status & TWI_M_WIF)
    {
        if (status & TWI_M_RXACK)
        {
            /* Slave NACKed – determine whether it was the address byte */
            if (s_state == TWI_STATE_SEND_ADDRESS)
                finish_transaction(TWI_RESULT_NACK_ADDR);
            else
                finish_transaction(TWI_RESULT_NACK_DATA);
            return;
        }

        /* ACK received – progress through state machine */
        switch (s_state)
        {
            case TWI_STATE_SEND_ADDRESS:
                /* Address ACKed; begin data phase */
                if (s_current->tx_len > 0u)
                {
                    s_state = TWI_STATE_WRITE_DATA;
                    handle_write_phase();           /* Send first data byte */
                }
                else if (s_current->rx_len > 0u)
                {
                    /* Read-only: address was sent with R bit – switch to read */
                    s_state = TWI_STATE_READ_DATA;
                    /* First byte will arrive triggering RIF */
                }
                else
                {
                    /* Zero-length transaction (bus probe) */
                    finish_transaction(TWI_RESULT_OK);
                }
                break;

            case TWI_STATE_WRITE_DATA:
                handle_write_phase();               /* Continue writing */
                break;

            case TWI_STATE_REPEATED_START:
                /* Repeated-START address ACKed; now read */
                s_state = TWI_STATE_READ_DATA;
                break;

            default:
                /* Unexpected WIF – abort */
                finish_transaction(TWI_RESULT_BUS_ERROR);
                break;
        }
    }

    /* ---- Read Interrupt Flag – data byte received ---- */
    if (status & TWI_M_RIF)
    {
        s_state = TWI_STATE_READ_DATA;
        handle_read_phase();
    }
}

/* -----------------------------------------------------------------------
 * Public API implementation
 * ----------------------------------------------------------------------- */

/**
 * @brief Initialise the TWI master hardware and engine state.
 */
void twi_master_init(void)
{
    s_state   = TWI_STATE_IDLE;          /* Engine starts idle */
    s_result  = TWI_RESULT_OK;
    s_current = NULL;
    s_tx_idx  = 0u;
    s_rx_idx  = 0u;

    twi_hw_master_enable();              /* Configure hardware (Layer 1) */
}

/**
 * @brief Submit a transaction to the engine.
 */
TWI_Result_t twi_master_submit(TWI_Transaction_t *t)
{
    /* Reject if engine is already busy */
    if (s_state != TWI_STATE_IDLE)
        return TWI_RESULT_BUSY;

    /* Store transaction context */
    s_current = t;
    s_tx_idx  = 0u;
    s_rx_idx  = 0u;
    s_result  = TWI_RESULT_OK;
    s_state   = TWI_STATE_SEND_ADDRESS;

    /* Determine R/W bit: if we have bytes to write, start with WRITE address */
    uint8_t addr_rw;
    if (t->tx_len > 0u)
        addr_rw = (uint8_t)((t->address << 1u) | 0x00u);  /* WRITE */
    else
        addr_rw = (uint8_t)((t->address << 1u) | 0x01u);  /* READ  */

    /* Trigger START – ISR takes over from here */
    twi_hw_start(addr_rw);

    return TWI_RESULT_OK;
}

/**
 * @brief Check whether a transaction is currently in progress.
 */
bool twi_master_busy(void)
{
    /* ATOMIC read of volatile state to avoid half-read on 8-bit AVR */
    bool busy;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        busy = (s_state != TWI_STATE_IDLE);
    }
    return busy;
}

/**
 * @brief Return the result of the last completed transaction.
 */
TWI_Result_t twi_master_result(void)
{
    TWI_Result_t r;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        r = s_result;
    }
    return r;
}

/**
 * @brief Block until the current transaction finishes (busy-wait helper).
 */
TWI_Result_t twi_master_wait(void)
{
    while (twi_master_busy())
    {
        /* Interrupts must be enabled; no WDT reset here for simplicity */
    }
    return twi_master_result();
}
