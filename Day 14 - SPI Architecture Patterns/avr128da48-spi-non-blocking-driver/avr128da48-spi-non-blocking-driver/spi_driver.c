/**
 * @file spi_nb.c
 * @brief Non-blocking SPI0 master driver implementation (state machine + ISR).
 */

#include "spi_driver.h"
#include <avr/io.h>
#include <avr/interrupt.h>

typedef struct
{
    volatile spi_state_t state;
    volatile spi_nb_status_t status;

    spi_device_id_t dev;
    const spi_device_cfg_t *cfg;

    const uint8_t *tx;
    uint8_t *rx;
    uint16_t len;
    volatile uint16_t idx;

    uint8_t filler;

    uint32_t timeout_ms;
    uint32_t start_ms;

    volatile uint8_t busy;
} spi_nb_ctx_t;

static spi_nb_ctx_t g_spi;

static uint8_t spi_mode_to_ctrlb(spi_mode_t mode)
{
#ifdef SPI_MODE_0_gc
    switch (mode)
    {
        case SPI_MODE0: return SPI_MODE_0_gc;
        case SPI_MODE1: return SPI_MODE_1_gc;
        case SPI_MODE2: return SPI_MODE_2_gc;
        case SPI_MODE3: return SPI_MODE_3_gc;
        default:        return SPI_MODE_0_gc;
    }
#else
    (void)mode;
    return 0;
#endif
}

static const spi_device_cfg_t *spi_get_cfg(spi_device_id_t dev)
{
    if ((uint8_t)dev >= (uint8_t)SPI_DEV_COUNT)
    {
        return (const spi_device_cfg_t *)0;
    }
    return &g_spi_devices[(uint8_t)dev];
}

static inline void spi_cs_assert(const spi_device_cfg_t *cfg)
{
    cfg->cs_port->OUTCLR = cfg->cs_pin_bm;
}

static inline void spi_cs_deassert(const spi_device_cfg_t *cfg)
{
    cfg->cs_port->OUTSET = cfg->cs_pin_bm;
}

static inline void spi_int_enable(void)
{
#ifdef SPI_IE_bm
    SPI0.INTCTRL = SPI_IE_bm;
#else
    SPI0.INTCTRL = 1;
#endif
}

static inline void spi_int_disable(void)
{
    SPI0.INTCTRL = 0;
}

static void spi_kick_transfer(void)
{
    g_spi.idx = 0;

    /* Clear IF flag */
    SPI0.INTFLAGS = SPI_IF_bm;

    /* Start first byte */
    uint8_t txb = (g_spi.tx != 0) ? g_spi.tx[0] : g_spi.filler;
    SPI0.DATA = txb;

    /* Enable interrupt for subsequent bytes */
    spi_int_enable();
}

spi_nb_status_t spi0_nb_init(void)
{
    /* Configure SPI pins */
    SPI0_PORT.DIRSET = SPI0_MOSI_bm | SPI0_SCK_bm;
    SPI0_PORT.DIRCLR = SPI0_MISO_bm;

    /* CS pins */
    SPI_DEV0_CS_PORT.DIRSET = SPI_DEV0_CS_bm;
    SPI_DEV0_CS_PORT.OUTSET = SPI_DEV0_CS_bm;

    SPI_DEV1_CS_PORT.DIRSET = SPI_DEV1_CS_bm;
    SPI_DEV1_CS_PORT.OUTSET = SPI_DEV1_CS_bm;

    /* Reset context */
    g_spi.state = SPI_STATE_IDLE;
    g_spi.status = SPI_NB_OK;
    g_spi.busy = 0;
    g_spi.filler = 0xFF;
    g_spi.cfg = 0;

    /* Configure SPI0 using DEV0 defaults */
    const spi_device_cfg_t *cfg = spi_get_cfg(SPI_DEV0);
    if (cfg == 0)
    {
        g_spi.status = SPI_NB_ERR_BAD_PARAM;
        g_spi.state = SPI_STATE_ERROR;
        return SPI_NB_ERR_BAD_PARAM;
    }

    SPI0.CTRLA = 0;
    SPI0.CTRLB = 0;

    SPI0.CTRLB = spi_mode_to_ctrlb(cfg->mode);
    SPI0.CTRLA = SPI_MASTER_bm | cfg->prescaler_gc | SPI_ENABLE_bm;

    spi_int_disable();
    SPI0.INTFLAGS = SPI_IF_bm;

    return SPI_NB_OK;
}

spi_nb_status_t spi0_nb_start(spi_device_id_t dev,
                              const uint8_t *tx_buf,
                              uint8_t *rx_buf,
                              uint16_t len,
                              uint32_t timeout_ms,
                              uint32_t now_ms)
{
    if (len == 0)
    {
        return SPI_NB_ERR_BAD_PARAM;
    }

    if (g_spi.busy)
    {
        return SPI_NB_ERR_BUSY;
    }

    const spi_device_cfg_t *cfg = spi_get_cfg(dev);
    if (cfg == 0)
    {
        return SPI_NB_ERR_BAD_PARAM;
    }

    g_spi.busy = 1;
    g_spi.dev = dev;
    g_spi.cfg = cfg;
    g_spi.tx = tx_buf;
    g_spi.rx = rx_buf;
    g_spi.len = len;
    g_spi.idx = 0;
    g_spi.timeout_ms = timeout_ms;
    g_spi.start_ms = now_ms;

    g_spi.status = SPI_NB_OK;
    g_spi.state = SPI_STATE_ASSERT_CS;

    return SPI_NB_OK;
}

void spi0_nb_task(uint32_t now_ms)
{
    /* Timeout check */
    if (g_spi.busy && (g_spi.timeout_ms != 0))
    {
        uint32_t elapsed = now_ms - g_spi.start_ms;
        if (elapsed >= g_spi.timeout_ms)
        {
            spi_int_disable();
            if (g_spi.cfg != 0)
            {
                spi_cs_deassert(g_spi.cfg);
            }

            g_spi.status = SPI_NB_ERR_TIMEOUT;
            g_spi.state = SPI_STATE_ERROR;
            g_spi.busy = 0;
            return;
        }
    }

    /* Advance non-ISR state */
    if (g_spi.state == SPI_STATE_ASSERT_CS)
    {
        /* Apply device mode/prescaler */
        SPI0.CTRLA &= ~SPI_ENABLE_bm;
        SPI0.CTRLB  = spi_mode_to_ctrlb(g_spi.cfg->mode);
        SPI0.CTRLA  = SPI_MASTER_bm | g_spi.cfg->prescaler_gc | SPI_ENABLE_bm;

        /* Assert CS */
        spi_cs_assert(g_spi.cfg);

        /* Start transfer */
        g_spi.state = SPI_STATE_TRANSFER;
        spi_kick_transfer();
    }
}

spi_state_t spi0_nb_get_state(void)
{
    return g_spi.state;
}

spi_nb_status_t spi0_nb_get_status(void)
{
    return g_spi.status;
}

void spi0_nb_reset(void)
{
    spi_int_disable();

    if (g_spi.cfg != 0)
    {
        spi_cs_deassert(g_spi.cfg);
    }

    g_spi.busy = 0;
    g_spi.state = SPI_STATE_IDLE;
    g_spi.status = SPI_NB_OK;

    g_spi.tx = 0;
    g_spi.rx = 0;
    g_spi.len = 0;
    g_spi.idx = 0;
    g_spi.timeout_ms = 0;
    g_spi.start_ms = 0;
}

/* SPI0 ISR: byte-by-byte transfer */
ISR(SPI0_INT_vect)
{
    if ((SPI0.INTFLAGS & SPI_IF_bm) == 0)
    {
        return;
    }

    SPI0.INTFLAGS = SPI_IF_bm;

    /* Read RX for the byte that just finished */
    uint8_t rxb = SPI0.DATA;

    if (g_spi.rx != 0)
    {
        g_spi.rx[g_spi.idx] = rxb;
    }

    g_spi.idx++;

    if (g_spi.idx >= g_spi.len)
    {
        /* Done */
        spi_int_disable();
        spi_cs_deassert(g_spi.cfg);

        g_spi.status = SPI_NB_OK;
        g_spi.state = SPI_STATE_COMPLETE;
        g_spi.busy = 0;
        return;
    }

    /* Load next TX byte */
    uint8_t txb = (g_spi.tx != 0) ? g_spi.tx[g_spi.idx] : g_spi.filler;
    SPI0.DATA = txb;
}
