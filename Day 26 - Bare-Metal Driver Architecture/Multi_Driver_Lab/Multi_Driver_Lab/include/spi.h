/*
 * spi.h  --  SPI driver public API, AVR128DA48
 *
 * Covers SPI0 and SPI1 in host (master) mode, normal (non-buffered) operation.
 *
 * Pin mapping (PORTMUX.SPIROUTEA):
 *   SPI0 DEFAULT : MOSI=PA4  MISO=PA5  SCK=PA6  SS=PA7
 *   SPI0 ALT1    : MOSI=PE0  MISO=PE1  SCK=PE2  SS=PE3
 *   SPI1 DEFAULT : MOSI=PC0  MISO=PC1  SCK=PC2  SS=PC3
 *   SPI1 ALT1    : MOSI=PC4  MISO=PC5  SCK=PC6  SS=PC7
 *
 * The application is responsible for:
 *   - Configuring MOSI and SCK as outputs, MISO as input, SS as output high
 *     before calling SPI_Init().
 *   - Asserting / de-asserting its own chip-select line(s) around transfers.
 */

#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

/* SPI instance selector */
typedef enum {
    SPI_INSTANCE_0 = 0,  /* SPI0 at 0x0940 */
    SPI_INSTANCE_1,      /* SPI1 at 0x0960 */
} spi_instance_t;

/* Clock prescaler  (SPI_PRESC_*_gc written to SPI.CTRLA bits [2:1]) */
typedef enum {
    SPI_PRESCALER_DIV4   = 0x00,  /* SPI_PRESC_DIV4_gc   */
    SPI_PRESCALER_DIV16  = 0x01,  /* SPI_PRESC_DIV16_gc  */
    SPI_PRESCALER_DIV64  = 0x02,  /* SPI_PRESC_DIV64_gc  */
    SPI_PRESCALER_DIV128 = 0x03,  /* SPI_PRESC_DIV128_gc */
} spi_prescaler_t;

/* SPI mode (CPOL / CPHA)  (SPI_MODE_*_gc written to SPI.CTRLB bits [1:0]) */
typedef enum {
    SPI_MODE_0 = 0x00,  /* CPOL=0 CPHA=0  SPI_MODE_0_gc */
    SPI_MODE_1 = 0x01,  /* CPOL=0 CPHA=1  SPI_MODE_1_gc */
    SPI_MODE_2 = 0x02,  /* CPOL=1 CPHA=0  SPI_MODE_2_gc */
    SPI_MODE_3 = 0x03,  /* CPOL=1 CPHA=1  SPI_MODE_3_gc */
} spi_mode_t;

/* Data order */
typedef enum {
    SPI_DATA_ORDER_MSB = 0,  /* MSB first (SPI_DORD_bm = 0) */
    SPI_DATA_ORDER_LSB = 1,  /* LSB first (SPI_DORD_bm = 1) */
} spi_data_order_t;

/* Driver configuration */
typedef struct {
    spi_instance_t   instance;
    spi_prescaler_t  prescaler;
    spi_mode_t       mode;
    spi_data_order_t data_order;
    uint8_t          clk2x;     /* Non-zero: enable CLK2X (SPI_CLK2X_bm) */
} spi_config_t;

/*
 * Public API
 */

/**
 * @brief  Initialise the selected SPI instance in host mode.
 * @param  cfg  Pointer to a filled spi_config_t.
 */
void    SPI_Init(const spi_config_t *cfg);

/**
 * @brief  Transmit and receive one byte simultaneously (full-duplex).
 * @param  inst  SPI instance.
 * @param  data  Byte to transmit.
 * @return Byte received during the transfer.
 */
uint8_t SPI_TransferByte(spi_instance_t inst, uint8_t data);

/**
 * @brief  Transmit a buffer; discards received bytes.
 * @param  inst   SPI instance.
 * @param  buf    Pointer to data to send.
 * @param  len    Number of bytes.
 */
void    SPI_Transmit(spi_instance_t inst, const uint8_t *buf, uint16_t len);

/**
 * @brief  Receive a buffer; transmits 0xFF as dummy bytes.
 * @param  inst   SPI instance.
 * @param  buf    Pointer to receive buffer.
 * @param  len    Number of bytes.
 */
void    SPI_Receive(spi_instance_t inst, uint8_t *buf, uint16_t len);

/**
 * @brief  Full-duplex transfer of a buffer.
 * @param  inst   SPI instance.
 * @param  tx     Transmit buffer (may be NULL to send 0xFF).
 * @param  rx     Receive  buffer (may be NULL to discard).
 * @param  len    Number of bytes.
 */
void    SPI_Transfer(spi_instance_t inst, const uint8_t *tx,
                     uint8_t *rx, uint16_t len);

/**
 * @brief  Disable the selected SPI instance.
 * @param  inst  SPI instance.
 */
void    SPI_Disable(spi_instance_t inst);

#endif /* SPI_H_ */
