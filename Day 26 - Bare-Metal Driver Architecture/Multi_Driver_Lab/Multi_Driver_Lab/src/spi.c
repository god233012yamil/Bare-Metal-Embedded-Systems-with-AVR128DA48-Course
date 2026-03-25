/*
 * spi.c  --  SPI driver implementation, AVR128DA48
 *
 * Peripheral : SPI0 (0x0940), SPI1 (0x0960)
 * Mode       : host (master), normal (non-buffered)
 * All register symbols from ioavr128da48.h via <avr/io.h>.
 *
 * SPI.CTRLA
 *   bit 0    : SPI_ENABLE_bm
 *   bits 2:1 : SPI_PRESC_gm  (SPI_PRESC_*_gc)
 *   bit 4    : SPI_CLK2X_bm
 *   bit 5    : SPI_MASTER_bm
 *   bit 6    : SPI_DORD_bm   (1 = LSB first)
 *
 * SPI.CTRLB
 *   bits 1:0 : SPI_MODE_gm   (SPI_MODE_*_gc)
 *   bit 2    : SPI_SSD_bm    (1 = disable SS pin, use SW CS)
 *
 * SPI.INTFLAGS
 *   bit 7    : SPI_IF_bm     (interrupt / transfer complete flag, non-buffered)
 *   bit 6    : SPI_WRCOL_bm  (write collision)
 */

#include "spi.h"
#include <avr/io.h>

/* Return a pointer to the hardware SPI struct for the given instance. */
static SPI_t *prv_inst(spi_instance_t inst)
{
    return (inst == SPI_INSTANCE_0) ? &SPI0 : &SPI1;
}

void SPI_Init(const spi_config_t *cfg)
{
    SPI_t *spi = prv_inst(cfg->instance);

    /* Disable before reconfiguring */
    spi->CTRLA = 0;

    /* CTRLB: set SPI mode, disable hardware SS (use software CS) */
    spi->CTRLB = (uint8_t)(cfg->mode) | SPI_SSD_bm;

    /* CTRLA: prescaler, CLK2X, master, data order, enable */
    uint8_t ctrla = SPI_ENABLE_bm
                  | SPI_MASTER_bm
                  | (uint8_t)((cfg->prescaler) << SPI_PRESC_gp);

    if (cfg->clk2x) {
        ctrla |= SPI_CLK2X_bm;
    }
    if (cfg->data_order == SPI_DATA_ORDER_LSB) {
        ctrla |= SPI_DORD_bm;
    }

    spi->CTRLA = ctrla;
}

uint8_t SPI_TransferByte(spi_instance_t inst, uint8_t data)
{
    SPI_t *spi = prv_inst(inst);
    spi->DATA = data;
    while (!(spi->INTFLAGS & SPI_IF_bm)) {}
    return spi->DATA;
}

void SPI_Transmit(spi_instance_t inst, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        SPI_TransferByte(inst, buf[i]);
    }
}

void SPI_Receive(spi_instance_t inst, uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = SPI_TransferByte(inst, 0xFF);
    }
}

void SPI_Transfer(spi_instance_t inst, const uint8_t *tx,
                  uint8_t *rx, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint8_t rxbyte = SPI_TransferByte(inst, tx ? tx[i] : 0xFF);
        if (rx) {
            rx[i] = rxbyte;
        }
    }
}

void SPI_Disable(spi_instance_t inst)
{
    prv_inst(inst)->CTRLA &= ~SPI_ENABLE_bm;
}
