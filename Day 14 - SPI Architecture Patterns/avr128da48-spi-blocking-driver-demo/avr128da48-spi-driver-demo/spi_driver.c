/**
 * @file spi_driver.c
 * @brief SPI0 master driver implementation for AVR128DA48.
 */

#include "spi_driver.h"
#include <avr/io.h>

/* Simple internal state: only one transaction at a time */
static volatile uint8_t g_spi_in_txn = 0;
static volatile spi_device_id_t g_spi_active_dev = (spi_device_id_t)0xFF;

/**
 * @brief Convert spi_mode_t to SPI0.CTRLB mode bits.
 *
 * @param mode SPI mode (0..3).
 * @return CTRLB mode bits.
 */
static uint8_t spi_mode_to_ctrlb(spi_mode_t mode)
{
    /* Mode bits are typically CPHA/CPOL. On AVR DA: SPI_MODE_..._gc values may exist.
       We'll set CPHA/CPOL based on mode using bitmasks if available.
       To keep this portable within AVR DA, use the CPHA/CPOL bit definitions. */

    uint8_t ctrlb = 0;

    /* CPHA: mode 1 and 3 */
    if ((mode == SPI_MODE1) || (mode == SPI_MODE3))
    {
        ctrlb |= SPI_SSD_bm; /* Not CPHA. Placeholder if CPHA bit differs on your headers. */
    }

    /* CPOL: mode 2 and 3 */
    if ((mode == SPI_MODE2) || (mode == SPI_MODE3))
    {
        ctrlb |= SPI_CLK2X_bm; /* Not CPOL. Placeholder if CPOL bit differs on your headers. */
    }

    /* IMPORTANT:
     * Different AVR header packs name CPHA/CPOL bits differently.
     * For Microchip AVR DA, the common approach is to use SPI0.CTRLB fields:
     * - SPI_MODE_0_gc, SPI_MODE_1_gc, etc. if defined.
     *
     * If your headers define SPI_MODE_0_gc .. SPI_MODE_3_gc, prefer that.
     */

#ifdef SPI_MODE_0_gc
    (void)ctrlb;
    switch (mode)
    {
        case SPI_MODE0: return SPI_MODE_0_gc;
        case SPI_MODE1: return SPI_MODE_1_gc;
        case SPI_MODE2: return SPI_MODE_2_gc;
        case SPI_MODE3: return SPI_MODE_3_gc;
        default:        return SPI_MODE_0_gc;
    }
#else
    /* Fallback: return 0 and rely on default mode 0.
       If your headers lack SPI_MODE_* enums, update this function to map CPHA/CPOL bits. */
    (void)ctrlb;
    return 0;
#endif
}

/**
 * @brief Assert CS for a device (active low).
 *
 * @param cfg Device config pointer.
 */
static inline void spi_cs_assert(const spi_device_cfg_t *cfg)
{
    /* Drive CS low */
    cfg->cs_port->OUTCLR = cfg->cs_pin_bm;
}

/**
 * @brief Deassert CS for a device (inactive high).
 *
 * @param cfg Device config pointer.
 */
static inline void spi_cs_deassert(const spi_device_cfg_t *cfg)
{
    /* Drive CS high */
    cfg->cs_port->OUTSET = cfg->cs_pin_bm;
}

/**
 * @brief Validate device ID and return config pointer.
 *
 * @param dev Device ID.
 * @return Pointer to config or NULL.
 */
static const spi_device_cfg_t *spi_get_dev_cfg(spi_device_id_t dev)
{
    if ((uint8_t)dev >= (uint8_t)SPI_DEV_COUNT)
    {
        return (const spi_device_cfg_t *)0;
    }
    return &g_spi_devices[(uint8_t)dev];
}

/**
 * @brief Wait for SPI transfer complete with timeout.
 *
 * @param timeout_loops Loop counter timeout (0 disables timeout protection).
 * @return SPI_OK or SPI_ERR_TIMEOUT.
 */
static spi_status_t spi_wait_if(uint32_t timeout_loops)
{
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0)
    {
        if (timeout_loops != 0)
        {
            timeout_loops--;
            if (timeout_loops == 0)
            {
                return SPI_ERR_TIMEOUT;
            }
        }
    }
    return SPI_OK;
}

spi_status_t spi0_init(void)
{
    /* Configure SPI pins */
    SPI0_PORT.DIRSET = SPI0_MOSI_bm | SPI0_SCK_bm;  /* MOSI + SCK outputs */
    SPI0_PORT.DIRCLR = SPI0_MISO_bm;                /* MISO input */

    /* Configure CS pins as outputs and set inactive high */
    SPI_DEV0_CS_PORT.DIRSET = SPI_DEV0_CS_bm;
    SPI_DEV0_CS_PORT.OUTSET = SPI_DEV0_CS_bm;

    SPI_DEV1_CS_PORT.DIRSET = SPI_DEV1_CS_bm;
    SPI_DEV1_CS_PORT.OUTSET = SPI_DEV1_CS_bm;

    /* Default to DEV0 config */
    const spi_device_cfg_t *cfg = spi_get_dev_cfg(SPI_DEV0);
    if (cfg == 0)
    {
        return SPI_ERR_BAD_PARAM;
    }

    /* Start from known state */
    SPI0.CTRLA = 0;
    SPI0.CTRLB = 0;

    /* Apply default mode and prescaler */
    SPI0.CTRLB = spi_mode_to_ctrlb(cfg->mode);

    SPI0.CTRLA = SPI_MASTER_bm          /* Master mode */
               | cfg->prescaler_gc      /* Prescaler */
               | SPI_ENABLE_bm;         /* Enable */

    /* Clear IF flag if set */
    SPI0.INTFLAGS = SPI_IF_bm;

    g_spi_in_txn = 0;
    g_spi_active_dev = (spi_device_id_t)0xFF;

    return SPI_OK;
}

spi_status_t spi0_begin(spi_device_id_t dev)
{
    const spi_device_cfg_t *cfg = spi_get_dev_cfg(dev);
    if (cfg == 0)
    {
        return SPI_ERR_BAD_PARAM;
    }

    /* Prevent overlapping transactions */
    if (g_spi_in_txn)
    {
        return SPI_ERR_BUSY;
    }

    /* Apply mode and prescaler for this device */
    SPI0.CTRLA &= ~SPI_ENABLE_bm;          /* Disable before changing settings */
    SPI0.CTRLB  = spi_mode_to_ctrlb(cfg->mode);
    SPI0.CTRLA  = SPI_MASTER_bm | cfg->prescaler_gc | SPI_ENABLE_bm;

    /* Ensure clean flag state */
    SPI0.INTFLAGS = SPI_IF_bm;

    /* Assert CS */
    spi_cs_assert(cfg);

    g_spi_in_txn = 1;
    g_spi_active_dev = dev;

    return SPI_OK;
}

spi_status_t spi0_end(spi_device_id_t dev)
{
    const spi_device_cfg_t *cfg = spi_get_dev_cfg(dev);
    if (cfg == 0)
    {
        return SPI_ERR_BAD_PARAM;
    }

    if (!g_spi_in_txn || (g_spi_active_dev != dev))
    {
        return SPI_ERR_BAD_PARAM;
    }

    /* Deassert CS */
    spi_cs_deassert(cfg);

    g_spi_in_txn = 0;
    g_spi_active_dev = (spi_device_id_t)0xFF;

    return SPI_OK;
}

spi_status_t spi0_transfer_byte(uint8_t tx, uint8_t *rx, uint32_t timeout_loops)
{
    if (!g_spi_in_txn)
    {
        return SPI_ERR_BAD_PARAM;
    }

    /* Start transfer */
    SPI0.DATA = tx;

    /* Wait for completion (timeout protected) */
    spi_status_t st = spi_wait_if(timeout_loops);
    if (st != SPI_OK)
    {
        return st;
    }

    /* Clear IF */
    SPI0.INTFLAGS = SPI_IF_bm;

    /* Read received byte */
    uint8_t r = SPI0.DATA;
    if (rx != 0)
    {
        *rx = r;
    }

    return SPI_OK;
}

spi_status_t spi0_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len, uint32_t timeout_loops)
{
    if (!g_spi_in_txn)
    {
        return SPI_ERR_BAD_PARAM;
    }

    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t tx = (tx_buf != 0) ? tx_buf[i] : 0xFF;
        uint8_t r  = 0;

        spi_status_t st = spi0_transfer_byte(tx, &r, timeout_loops);
        if (st != SPI_OK)
        {
            return st;
        }

        if (rx_buf != 0)
        {
            rx_buf[i] = r;
        }
    }

    return SPI_OK;
}

spi_status_t spi0_transaction(spi_device_id_t dev, const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len, uint32_t timeout_loops)
{
    spi_status_t st = spi0_begin(dev);
    if (st != SPI_OK)
    {
        return st;
    }

    st = spi0_transfer_buf(tx_buf, rx_buf, len, timeout_loops);

    /* Always end transaction */
    (void)spi0_end(dev);

    return st;
}
