/**
 * @file board.h
 * @brief Board-level pin mapping and SPI device configuration for AVR128DA48.
 */

#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU (24000000UL)
#endif

#define LED_PORT        PORTA
#define LED_PIN_bm      PIN2_bm

#define SPI0_PORT       PORTA
#define SPI0_MOSI_bm    PIN4_bm
#define SPI0_MISO_bm    PIN5_bm
#define SPI0_SCK_bm     PIN6_bm

typedef enum
{
    SPI_DEV0 = 0,
    SPI_DEV1 = 1,
    SPI_DEV_COUNT
} spi_device_id_t;

/* DEV0 CS: PA7 */
#define SPI_DEV0_CS_PORT PORTA
#define SPI_DEV0_CS_bm   PIN7_bm

/* DEV1 CS: PB0 */
#define SPI_DEV1_CS_PORT PORTB
#define SPI_DEV1_CS_bm   PIN0_bm

typedef enum
{
    SPI_MODE0 = 0,
    SPI_MODE1 = 1,
    SPI_MODE2 = 2,
    SPI_MODE3 = 3
} spi_mode_t;

typedef struct
{
    spi_device_id_t id;
    spi_mode_t mode;
    uint8_t prescaler_gc;     /* SPI_PRESC_*_gc value */
    volatile PORT_t *cs_port;
    uint8_t cs_pin_bm;
} spi_device_cfg_t;

static const spi_device_cfg_t g_spi_devices[SPI_DEV_COUNT] =
{
    { SPI_DEV0, SPI_MODE0, SPI_PRESC_DIV16_gc, &SPI_DEV0_CS_PORT, SPI_DEV0_CS_bm },
    { SPI_DEV1, SPI_MODE3, SPI_PRESC_DIV64_gc, &SPI_DEV1_CS_PORT, SPI_DEV1_CS_bm }
};

#endif /* BOARD_H */
