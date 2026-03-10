/**
 * @file twi_nb.c
 * @brief Non-blocking TWI0 (I2C) master driver implementation for AVR128DA48.
 *
 * Implementation details:
 * - ISR (TWI0_TWIM_vect) advances the transfer byte-by-byte using WIF/RIF events
 * - twi0_task() handles timeout supervision
 * - STOP is issued on completion or error
 */

#include "twi_nb.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

typedef struct
{
    volatile twi_state_t state;
    volatile twi_status_t status;

    uint8_t addr;

    const uint8_t *tx;
    uint16_t tx_len;
    volatile uint16_t tx_idx;

    uint8_t *rx;
    uint16_t rx_len;
    volatile uint16_t rx_idx;

    uint8_t do_read_phase;

    uint32_t timeout_ms;
    uint32_t start_ms;

    volatile uint8_t busy;
} twi_ctx_t;

static twi_ctx_t g_twi;

/**
 * @brief Compute MBAUD value for TWI master.
 *
 * Approximation:
 * MBAUD = (F_CPU / (2 * F_SCL)) - 5
 *
 * @param f_cpu_hz Peripheral clock in Hz.
 * @param i2c_hz Desired SCL in Hz.
 * @return MBAUD (8-bit).
 */
static uint8_t twi_calc_mbaud(uint32_t f_cpu_hz, uint32_t i2c_hz)
{
    uint32_t v;

    if (i2c_hz == 0u)
    {
        return 0u;
    }

    v = (f_cpu_hz / (2u * i2c_hz));

    if (v > 5u)
    {
        v -= 5u;
    }

    if (v > 255u)
    {
        v = 255u;
    }

    return (uint8_t)v;
}

/**
 * @brief Force STOP and transition to ERROR.
 *
 * @param err Error code to store.
 */
static void twi_force_stop_error(twi_status_t err)
{
    /* Issue STOP */
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    /* Store status/state */
    g_twi.status = err;
    g_twi.state = TWI_STATE_ERROR;

    /* Mark not busy */
    g_twi.busy = 0;
}

/**
 * @brief Finish transaction successfully (STOP + COMPLETE).
 */
static void twi_finish_ok(void)
{
    /* Issue STOP */
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    /* Store status/state */
    g_twi.status = TWI_OK;
    g_twi.state = TWI_STATE_COMPLETE;

    /* Mark not busy */
    g_twi.busy = 0;
}

/**
 * @brief Send START + SLA+W.
 */
static void twi_send_addr_write(void)
{
    /* Send address in write direction */
    TWI0.MADDR = (uint8_t)(g_twi.addr << 1);
    g_twi.state = TWI_STATE_ADDR_W;
}

/**
 * @brief Send START (or repeated START) + SLA+R.
 */
static void twi_send_addr_read(void)
{
    /* Send address in read direction */
    TWI0.MADDR = (uint8_t)((g_twi.addr << 1) | 0x01u);
    g_twi.state = TWI_STATE_ADDR_R;
}

twi_status_t twi0_init(uint32_t f_cpu_hz, uint32_t i2c_hz)
{
    /* Reset context */
    g_twi.state = TWI_STATE_IDLE;
    g_twi.status = TWI_OK;
    g_twi.busy = 0;

    /* Configure baud */
    TWI0.MBAUD = twi_calc_mbaud(f_cpu_hz, i2c_hz);

    /* Enable TWI master and interrupts */
    TWI0.MCTRLA = TWI_ENABLE_bm | TWI_RIEN_bm | TWI_WIEN_bm;

    /* Force bus state idle */
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

    /* Clear flags */
    TWI0.MSTATUS |= (TWI_WIF_bm | TWI_RIF_bm | TWI_BUSERR_bm | TWI_ARBLOST_bm);

    return TWI_OK;
}

twi_status_t twi0_start_write(uint8_t addr_7bit,
                              const uint8_t *tx_buf,
                              uint16_t tx_len,
                              uint32_t timeout_ms,
                              uint32_t now_ms)
{
    if ((tx_buf == 0) || (tx_len == 0u))
    {
        return TWI_ERR_BAD_PARAM;
    }

    if (g_twi.busy)
    {
        return TWI_ERR_BUSY;
    }

    g_twi.busy = 1;
    g_twi.addr = addr_7bit;

    g_twi.tx = tx_buf;
    g_twi.tx_len = tx_len;
    g_twi.tx_idx = 0;

    g_twi.rx = 0;
    g_twi.rx_len = 0;
    g_twi.rx_idx = 0;

    g_twi.do_read_phase = 0;

    g_twi.timeout_ms = timeout_ms;
    g_twi.start_ms = now_ms;

    g_twi.status = TWI_OK;
    g_twi.state = TWI_STATE_START;

    /* Kick off address write */
    twi_send_addr_write();

    return TWI_OK;
}

twi_status_t twi0_start_read(uint8_t addr_7bit,
                             uint8_t *rx_buf,
                             uint16_t rx_len,
                             uint32_t timeout_ms,
                             uint32_t now_ms)
{
    if ((rx_buf == 0) || (rx_len == 0u))
    {
        return TWI_ERR_BAD_PARAM;
    }

    if (g_twi.busy)
    {
        return TWI_ERR_BUSY;
    }

    g_twi.busy = 1;
    g_twi.addr = addr_7bit;

    g_twi.tx = 0;
    g_twi.tx_len = 0;
    g_twi.tx_idx = 0;

    g_twi.rx = rx_buf;
    g_twi.rx_len = rx_len;
    g_twi.rx_idx = 0;

    g_twi.do_read_phase = 0;

    g_twi.timeout_ms = timeout_ms;
    g_twi.start_ms = now_ms;

    g_twi.status = TWI_OK;
    g_twi.state = TWI_STATE_START;

    /* Kick off address read */
    twi_send_addr_read();

    return TWI_OK;
}

twi_status_t twi0_start_write_read(uint8_t addr_7bit,
                                   const uint8_t *tx_buf,
                                   uint16_t tx_len,
                                   uint8_t *rx_buf,
                                   uint16_t rx_len,
                                   uint32_t timeout_ms,
                                   uint32_t now_ms)
{
    if ((tx_buf == 0) || (tx_len == 0u) || (rx_buf == 0) || (rx_len == 0u))
    {
        return TWI_ERR_BAD_PARAM;
    }

    if (g_twi.busy)
    {
        return TWI_ERR_BUSY;
    }

    g_twi.busy = 1;
    g_twi.addr = addr_7bit;

    g_twi.tx = tx_buf;
    g_twi.tx_len = tx_len;
    g_twi.tx_idx = 0;

    g_twi.rx = rx_buf;
    g_twi.rx_len = rx_len;
    g_twi.rx_idx = 0;

    g_twi.do_read_phase = 1;

    g_twi.timeout_ms = timeout_ms;
    g_twi.start_ms = now_ms;

    g_twi.status = TWI_OK;
    g_twi.state = TWI_STATE_START;

    /* First phase: address write */
    twi_send_addr_write();

    return TWI_OK;
}

void twi0_task(uint32_t now_ms)
{
    if (g_twi.busy && (g_twi.timeout_ms != 0u))
    {
        uint32_t elapsed = (uint32_t)(now_ms - g_twi.start_ms);

        if (elapsed >= g_twi.timeout_ms)
        {
            /* Timeout */
            twi_force_stop_error(TWI_ERR_TIMEOUT);
        }
    }
}

twi_state_t twi0_get_state(void)
{
    return g_twi.state;
}

twi_status_t twi0_get_status(void)
{
    return g_twi.status;
}

void twi0_reset(void)
{
    /* Send STOP for safety */
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;

    /* Clear context */
    g_twi.busy = 0;
    g_twi.state = TWI_STATE_IDLE;
    g_twi.status = TWI_OK;

    g_twi.addr = 0u;

    g_twi.tx = 0;
    g_twi.tx_len = 0u;
    g_twi.tx_idx = 0u;

    g_twi.rx = 0;
    g_twi.rx_len = 0u;
    g_twi.rx_idx = 0u;

    g_twi.do_read_phase = 0u;
    g_twi.timeout_ms = 0u;
    g_twi.start_ms = 0u;
}

/**
 * @brief TWI0 Master ISR advances the transaction state machine.
 */
ISR(TWI0_TWIM_vect)
{
    uint8_t st = TWI0.MSTATUS;

    /* Bus error */
    if (st & TWI_BUSERR_bm)
    {
        /* Clear flag */
        TWI0.MSTATUS = TWI_BUSERR_bm;
        twi_force_stop_error(TWI_ERR_BUS);
        return;
    }

    /* Arbitration lost */
    if (st & TWI_ARBLOST_bm)
    {
        /* Clear flag */
        TWI0.MSTATUS = TWI_ARBLOST_bm;
        twi_force_stop_error(TWI_ERR_ARBLOST);
        return;
    }

    /* Write event */
    if (st & TWI_WIF_bm)
    {
        /* Clear WIF */
        TWI0.MSTATUS = TWI_WIF_bm;

        /* NACK check */
        if (st & TWI_RXACK_bm)
        {
            twi_force_stop_error(TWI_ERR_NACK);
            return;
        }

        /* If there is write data remaining, send it */
        if ((g_twi.tx != 0) && (g_twi.tx_idx < g_twi.tx_len))
        {
            /* Send next byte */
            TWI0.MDATA = g_twi.tx[g_twi.tx_idx];
            g_twi.tx_idx++;
            g_twi.state = TWI_STATE_WRITE;
            return;
        }

        /* Write phase complete */
        if (g_twi.do_read_phase && (g_twi.rx_len != 0u))
        {
            /* Repeated START then address read */
            g_twi.state = TWI_STATE_REP_START;

            /* Issue repeated start */
            TWI0.MCTRLB = TWI_MCMD_REPSTART_gc;

            /* Send SLA+R */
            twi_send_addr_read();
            return;
        }

        /* No read phase: complete */
        g_twi.state = TWI_STATE_STOP;
        twi_finish_ok();
        return;
    }

    /* Read event */
    if (st & TWI_RIF_bm)
    {
        /* Clear RIF */
        TWI0.MSTATUS = TWI_RIF_bm;

        /* Read data */
        uint8_t d = TWI0.MDATA;

        if ((g_twi.rx != 0) && (g_twi.rx_idx < g_twi.rx_len))
        {
            g_twi.rx[g_twi.rx_idx] = d;
        }

        g_twi.rx_idx++;
        g_twi.state = TWI_STATE_READ;

        /* More bytes to read? */
        if (g_twi.rx_idx < g_twi.rx_len)
        {
            /* If next byte is last, prepare NACK on that receive */
            if (g_twi.rx_idx == (g_twi.rx_len - 1u))
            {
                TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_RECVTRANS_gc;
            }
            else
            {
                TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
            }
            return;
        }

        /* All bytes read: stop and complete */
        g_twi.state = TWI_STATE_STOP;
        twi_finish_ok();
        return;
    }

    /* No recognized flags; ignore */
}
