/**
 * @file main.c
 * @brief Demo application for the SPI0 transaction-based driver (AVR128DA48).
 *
 * Demo steps:
 * 1) Initializes LED GPIO and SPI driver.
 * 2) Runs a loopback test on SPI_DEV0:
 *    - Requires a wire from MOSI to MISO.
 *    - Transfers a known pattern and checks the received bytes.
 * 3) Demonstrates a "JEDEC ID read" command (0x9F) as an example transaction.
 *
 * LED behavior:
 * - Fast blink: loopback OK
 * - Slow blink: loopback FAIL (or SPI timeout)
 */

#include <avr/io.h>
#include <stdint.h>
#include "board.h"
#include "spi_driver.h"

/* Simple delay for visible LED patterns (not precise) */
static void delay_loops(volatile uint32_t loops)
{
    while (loops--)
    {
        __asm__ __volatile__("nop");
    }
}

static void led_init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;   /* LED pin as output */
    LED_PORT.OUTCLR = LED_PIN_bm;   /* LED off */
}

static void led_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/* Loopback test (MOSI wired to MISO) */
static uint8_t spi_loopback_test(void)
{
    const uint8_t tx[] = { 0xAA, 0x55, 0x12, 0x34, 0x00, 0xFF };
    uint8_t rx[sizeof(tx)];

    spi_status_t st = spi0_transaction(SPI_DEV0, tx, rx, (uint16_t)sizeof(tx), 200000UL);
    if (st != SPI_OK)
    {
        return 0;
    }

    for (uint8_t i = 0; i < (uint8_t)sizeof(tx); i++)
    {
        if (rx[i] != tx[i])
        {
            return 0;
        }
    }
    return 1;
}

/* Example: Read JEDEC ID (0x9F) from an SPI flash if connected */
static uint32_t spi_read_jedec_id(void)
{
    uint8_t tx[4] = { 0x9F, 0xFF, 0xFF, 0xFF };
    uint8_t rx[4] = { 0, 0, 0, 0 };

    (void)spi0_transaction(SPI_DEV0, tx, rx, 4, 200000UL);

    return ((uint32_t)rx[1] << 16)
         | ((uint32_t)rx[2] << 8)
         | ((uint32_t)rx[3]);
}

int main(void)
{
    led_init();

    // Initialize SPI0 driver (GPIO + peripheral) 
    (void)spi0_init();

    // Run one-time loopback test 
    uint8_t ok = spi_loopback_test();

    // Read JEDEC ID (optional external device)
    volatile uint32_t jedec_id = spi_read_jedec_id();
    (void)jedec_id;

    while (1)
    {
        led_toggle();

        if (ok)
        {
            // Fast blink if loopback is OK
            delay_loops(120000UL);
        }
        else
        {
            // Slow blink if loopback failed
            delay_loops(600000UL);
        }
    }
}