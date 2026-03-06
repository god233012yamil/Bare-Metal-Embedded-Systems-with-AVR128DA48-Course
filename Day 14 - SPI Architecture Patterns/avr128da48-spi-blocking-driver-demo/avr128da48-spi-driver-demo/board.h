/**
 * @file board.h
 * @brief Board-level pin mapping and SPI device configuration for AVR128DA48.
 *
 * Adjust these definitions to match your wiring, board routing, and PORTMUX setup.
 */

#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>
#include <stdint.h>

/* ----------------------------- */
/* CPU Clock                     */
/* ----------------------------- */
#ifndef F_CPU
#define F_CPU (4000000UL)
#endif

/* ----------------------------- */
/* LED (demo)                    */
/* ----------------------------- */
#define LED_PORT        PORTA
#define LED_PIN_bm      PIN2_bm

/* ----------------------------- */
/* SPI0 Pins (example)           */
/* ----------------------------- */
#define SPI0_PORT       PORTA
#define SPI0_MOSI_bm    PIN4_bm
#define SPI0_MISO_bm    PIN5_bm
#define SPI0_SCK_bm     PIN6_bm

/* ----------------------------- */
/* SPI Devices (chip selects)    */
/* ----------------------------- */
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

/* ----------------------------- */
/* SPI Modes                     */
/* ----------------------------- */
typedef enum
{
    SPI_MODE0 = 0, /* CPOL=0, CPHA=0 */
    SPI_MODE1 = 1, /* CPOL=0, CPHA=1 */
    SPI_MODE2 = 2, /* CPOL=1, CPHA=0 */
    SPI_MODE3 = 3  /* CPOL=1, CPHA=1 */
} spi_mode_t;

/* ----------------------------- */
/* Per-device configuration      */
/* ----------------------------- */
typedef struct
{
    spi_device_id_t id;
    spi_mode_t mode;
    uint8_t prescaler_gc; /* SPI_PRESC_*_gc enum value */
    volatile PORT_t *cs_port;
    uint8_t cs_pin_bm;
} spi_device_cfg_t;

/*
 * Device table used by the driver.
 * You can set different modes/prescalers per device.
 */
static const spi_device_cfg_t g_spi_devices[SPI_DEV_COUNT] =
{
    /* id       mode       prescaler               cs_port            cs_pin */
    { SPI_DEV0, SPI_MODE0, SPI_PRESC_DIV16_gc,     &SPI_DEV0_CS_PORT, SPI_DEV0_CS_bm },
    { SPI_DEV1, SPI_MODE3, SPI_PRESC_DIV64_gc,     &SPI_DEV1_CS_PORT, SPI_DEV1_CS_bm }
};

#endif /* BOARD_H */
