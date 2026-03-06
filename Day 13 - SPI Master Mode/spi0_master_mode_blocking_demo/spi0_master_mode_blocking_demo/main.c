/**
 * @file main.c
 * @brief SPI0 Master mode example for AVR128DA48 (bare metal, register-level).
 *
 * This example demonstrates a complete, minimal SPI0 Master implementation:
 * - Manual chip-select (CS) using a GPIO pin
 * - Blocking byte transfer routine
 * - Simple buffer read/write helpers
 * - A loop that repeatedly sends a test pattern (0xAA, 0x55)
 *
 * Notes:
 * - Pin mapping is board/routing dependent. This file uses example pins on PORTA.
 * - You must adjust MOSI/MISO/SCK/CS pins and PORTMUX routing if your board uses
 *   a different pinout for SPI0.
 *
 * Example pin assumptions (adjust as needed):
 * - MOSI: PA4
 * - MISO: PA5
 * - SCK : PA6
 * - CS  : PA7 (manual GPIO)
 *
 * Test method:
 * - Connect a logic analyzer or scope to CS, SCK, MOSI.
 * - You should see CS go low, then clock pulses, then MOSI bits for 0xAA and 0x55.
 */

#include <avr/io.h>
#include <stdint.h>

/* ============================= */
/* Configuration                 */
/* ============================= */

/* Example GPIO mapping on PORTA (adjust to match your wiring/board routing). */
#define SPI0_PORT        PORTA
#define SPI0_MOSI_bm     PIN4_bm
#define SPI0_MISO_bm     PIN5_bm
#define SPI0_SCK_bm      PIN6_bm
#define SPI0_CS_bm       PIN7_bm

/* Choose an SPI prescaler. Start slow, then increase after validation. */
#define SPI0_PRESC_CFG   SPI_PRESC_DIV16_gc

/* ============================= */
/* Local Function Prototypes     */
/* ============================= */

static void spi0_gpio_init(void);
static void spi0_init_master_mode0(void);
static inline void spi0_cs_assert(void);
static inline void spi0_cs_deassert(void);
static uint8_t spi0_transfer_byte(uint8_t tx);
static void spi0_write_buffer(const uint8_t *buf, uint16_t len);
static void spi0_read_buffer(uint8_t *buf, uint16_t len, uint8_t filler);
static void simple_delay(volatile uint32_t loops);

/* ============================= */
/* GPIO + SPI Initialization     */
/* ============================= */

/**
 * @brief Initialize GPIO directions for SPI0 pins and CS.
 *
 * This configures:
 * - MOSI and SCK as outputs (driven by master)
 * - MISO as input (driven by slave)
 * - CS as output (manual control by firmware)
 * - CS set to inactive high
 */
static void spi0_gpio_init(void)
{
    /* MOSI, SCK, CS as outputs */
    SPI0_PORT.DIRSET = SPI0_MOSI_bm | SPI0_SCK_bm | SPI0_CS_bm;

    /* MISO as input */
    SPI0_PORT.DIRCLR = SPI0_MISO_bm;

    /* Default CS inactive (high) */
    SPI0_PORT.OUTSET = SPI0_CS_bm;
}

/**
 * @brief Initialize SPI0 in Master mode, SPI Mode 0 (CPOL=0, CPHA=0).
 *
 * Mode 0:
 * - Clock idles low
 * - Data is sampled on the rising edge
 *
 * The prescaler is configured by SPI0_PRESC_CFG.
 */
static void spi0_init_master_mode0(void)
{
    /*
     * CTRLB typically holds mode-related options on AVR DA.
     * We'll explicitly clear it to start from a known state.
     */
    SPI0.CTRLB = 0;

    /*
     * Enable SPI0 as Master.
     * Select prescaler.
     * Enable SPI peripheral.
     *
     * If your device supports additional fields (like CLK2X),
     * keep them off initially until you validate waveforms.
     */
    SPI0.CTRLA = SPI_MASTER_bm     /* Master mode */
               | SPI0_PRESC_CFG    /* Clock prescaler */
               | SPI_ENABLE_bm;    /* Enable SPI */
}

/* ============================= */
/* Chip Select Control           */
/* ============================= */

/**
 * @brief Assert chip select (active low).
 *
 * Drives CS low to select the slave.
 */
static inline void spi0_cs_assert(void)
{
    /* CS low selects slave */
    SPI0_PORT.OUTCLR = SPI0_CS_bm;
}

/**
 * @brief Deassert chip select (inactive high).
 *
 * Drives CS high to deselect the slave.
 */
static inline void spi0_cs_deassert(void)
{
    /* CS high deselects slave */
    SPI0_PORT.OUTSET = SPI0_CS_bm;
}

/* ============================= */
/* SPI Transfer Routines         */
/* ============================= */

/**
 * @brief Transfer a single byte over SPI0 (blocking).
 *
 * SPI is full duplex: while transmitting tx, a byte is received simultaneously.
 *
 * @param tx Byte to transmit.
 * @return Byte received from the slave during the transfer.
 */
static uint8_t spi0_transfer_byte(uint8_t tx)
{
    /* Writing DATA starts a transfer */
    SPI0.DATA = tx;

    /* Wait for transfer complete flag */
    while ((SPI0.INTFLAGS & SPI_IF_bm) == 0)
    {
        /* Busy wait */
    }

    /* Clear the interrupt flag by writing 1 to it */
    SPI0.INTFLAGS = SPI_IF_bm;

    /* Read and return received byte */
    return SPI0.DATA;
}

/**
 * @brief Write a buffer to the SPI slave (blocking).
 *
 * @param buf Pointer to transmit buffer.
 * @param len Number of bytes to transmit.
 */
static void spi0_write_buffer(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        (void)spi0_transfer_byte(buf[i]); /* ignore RX for write-only transfer */
    }
}

/**
 * @brief Read a buffer from the SPI slave (blocking).
 *
 * To read bytes on SPI, the master must generate clock pulses by transmitting
 * dummy filler bytes (often 0xFF or 0x00). The received bytes are captured.
 *
 * @param buf Pointer to receive buffer.
 * @param len Number of bytes to read.
 * @param filler Byte to transmit while reading (commonly 0xFF).
 */
static void spi0_read_buffer(uint8_t *buf, uint16_t len, uint8_t filler)
{
    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = spi0_transfer_byte(filler);
    }
}

/* ============================= */
/* Utility                       */
/* ============================= */

/**
 * @brief Simple software delay loop (not precise).
 *
 * This is used only to slow down the demo loop so signals are easy to observe
 * on a scope or logic analyzer.
 *
 * @param loops Loop count. Higher means longer delay.
 */
static void simple_delay(volatile uint32_t loops)
{
    while (loops--)
    {
        /* prevent optimization; do nothing */
        __asm__ __volatile__("nop");
    }
}

/* ============================= */
/* Main                          */
/* ============================= */

/**
 * @brief Application entry point.
 *
 * Initializes GPIO and SPI0 in Master mode, then repeatedly:
 * - Asserts CS
 * - Transfers two bytes (0xAA, 0x55)
 * - Deasserts CS
 * - Waits a bit
 */
int main(void)
{
    /* Configure SPI pins and CS pin */
    spi0_gpio_init();

    /* Configure SPI0 as Master, Mode 0 */
    spi0_init_master_mode0();

    /* Example test pattern */
    const uint8_t pattern[] = { 0xAA, 0x55 };

    while (1)
    {
        /* Select the slave */
        spi0_cs_assert();

        /* Transmit a known pattern */
        spi0_write_buffer(pattern, (uint16_t)sizeof(pattern));

        /* Optional: read back two bytes (will depend on your slave device) */
        /* uint8_t rx[2]; */
        /* spi0_read_buffer(rx, 2, 0xFF); */

        /* Deselect the slave */
        spi0_cs_deassert();

        /* Slow down loop for easier measurement */
        simple_delay(200000UL);
    }
}