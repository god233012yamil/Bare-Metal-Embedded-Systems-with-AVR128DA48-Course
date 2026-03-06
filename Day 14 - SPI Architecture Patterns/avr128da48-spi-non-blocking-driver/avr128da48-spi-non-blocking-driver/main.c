/**
 * @file main.c
 * @brief Demo for the non-blocking SPI0 driver (state machine + ISR).
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "board.h"
#include "spi_driver.h"

static volatile uint32_t g_ms_ticks = 0;

static void tick_init_1ms(void)
{
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = (uint16_t)((F_CPU / 1000UL) - 1UL);
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm;
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    g_ms_ticks++;
}

static void led_init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;
    LED_PORT.OUTCLR = LED_PIN_bm;
}

static void led_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

static void fill_pattern(uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)(i ^ 0xA5);
    }
}

static uint8_t buffers_match(const uint8_t *a, const uint8_t *b, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if (a[i] != b[i])
        {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    led_init();
    tick_init_1ms();

    (void)spi0_nb_init();

    sei();

    uint8_t tx_small[16];
    uint8_t rx_small[16];
    fill_pattern(tx_small, (uint16_t)sizeof(tx_small));

    static uint8_t tx_big[2048];
    static uint8_t rx_big[2048];
    fill_pattern(tx_big, (uint16_t)sizeof(tx_big));

    uint8_t ok_small = 0;
    uint8_t ok_big = 0;

    /* Start small transfer */
    (void)spi0_nb_start(SPI_DEV0, tx_small, rx_small, (uint16_t)sizeof(tx_small), 200UL, g_ms_ticks);

    uint32_t last_led = g_ms_ticks;

    while (1)
    {
        /* Cooperative task advances ASSERT_CS and checks timeout */
        spi0_nb_task(g_ms_ticks);

        /* Heartbeat: main keeps running while SPI transfers in ISR */
        if ((g_ms_ticks - last_led) >= 100UL)
        {
            last_led = g_ms_ticks;
            led_toggle();
        }

        if (spi0_nb_get_state() == SPI_STATE_COMPLETE)
        {
            /* Determine which transfer just completed */
            if (!ok_small)
            {
                ok_small = buffers_match(tx_small, rx_small, (uint16_t)sizeof(tx_small));
                spi0_nb_reset();

                /* Start long transfer */
                (void)spi0_nb_start(SPI_DEV0, tx_big, rx_big, (uint16_t)sizeof(tx_big), 5000UL, g_ms_ticks);
            }
            else if (!ok_big)
            {
                ok_big = buffers_match(tx_big, rx_big, (uint16_t)sizeof(tx_big));
                spi0_nb_reset();
            }
            else
            {
                /* Both tests done, keep blinking */
                spi0_nb_reset();
            }
        }
        else if (spi0_nb_get_state() == SPI_STATE_ERROR)
        {
            /* Error: stop transfers; keep blinking */
            (void)spi0_nb_get_status();
            spi0_nb_reset();
            ok_small = 0;
            ok_big = 0;
        }

        (void)ok_small;
        (void)ok_big;
    }
}