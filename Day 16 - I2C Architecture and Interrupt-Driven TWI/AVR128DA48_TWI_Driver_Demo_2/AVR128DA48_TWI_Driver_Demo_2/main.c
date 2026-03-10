/**
 * @file main.c
 * @brief Demo application for non-blocking TWI0 driver (AVR128DA48 Curiosity Nano).
 *
 * LED behavior:
 * - Fast blink when last transaction succeeded
 * - Slow blink when last transaction failed (NACK/timeout/bus error)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "board.h"
#include "twi_nb.h"

static volatile uint32_t g_ms_ticks = 0;

/**
 * @brief Initialize TCB0 to generate a 1 ms tick.
 */
static void tick_init_1ms(void)
{
    /* Disable timer before config */
    TCB0.CTRLA = 0;

    /* Periodic interrupt mode */
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;

    /* 1 ms interval */
    TCB0.CCMP = (uint16_t)((F_CPU / 1000UL) - 1UL);

    /* Clear pending flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;

    /* Enable interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Enable timer */
    TCB0.CTRLA = TCB_ENABLE_bm;
}

/**
 * @brief TCB0 ISR increments millisecond tick counter.
 */
ISR(TCB0_INT_vect)
{
    /* Clear interrupt flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;

    /* Increment tick */
    g_ms_ticks++;
}

int main(void)
{
    /* Board init */
    board_led_init();
    board_init_i2c_pins();

    /* Time base */
    tick_init_1ms();

    /* TWI init at 100 kHz */
    (void)twi0_init((uint32_t)F_CPU, 100000UL);

    /* Enable interrupts */
    sei();

    /* Transaction buffers */
    uint8_t reg = (uint8_t)DEMO_I2C_REG;
    uint8_t rx[DEMO_I2C_READ_LEN];

    uint8_t last_ok = 0;

    /* Start first transaction */
    (void)twi0_start_write_read((uint8_t)DEMO_I2C_ADDR,
                                &reg, 1,
                                rx, (uint16_t)DEMO_I2C_READ_LEN,
                                100UL,
                                g_ms_ticks);

    uint32_t last_led = g_ms_ticks;
    uint32_t last_restart = g_ms_ticks;

    while (1)
    {
        /* Cooperative supervision */
        twi0_task(g_ms_ticks);

        /* LED blink pattern based on last result */
        if (last_ok)
        {
            if ((g_ms_ticks - last_led) >= 100UL)
            {
                last_led = g_ms_ticks;
                board_led_toggle();
            }
        }
        else
        {
            if ((g_ms_ticks - last_led) >= 400UL)
            {
                last_led = g_ms_ticks;
                board_led_toggle();
            }
        }

        /* Check transaction completion */
        if (twi0_get_state() == TWI_STATE_COMPLETE)
        {
            /* Success: inspect rx[] in debugger */
            last_ok = 1;

            /* Reset to idle */
            twi0_reset();
            last_restart = g_ms_ticks;
        }
        else if (twi0_get_state() == TWI_STATE_ERROR)
        {
            /* Failure: inspect twi0_get_status() in debugger */
            (void)twi0_get_status();
            last_ok = 0;

            /* Reset to idle */
            twi0_reset();
            last_restart = g_ms_ticks;
        }

        /* Restart periodically */
        if ((twi0_get_state() == TWI_STATE_IDLE) && ((g_ms_ticks - last_restart) >= 500UL))
        {
            (void)twi0_start_write_read((uint8_t)DEMO_I2C_ADDR,
                                        &reg, 1,
                                        rx, (uint16_t)DEMO_I2C_READ_LEN,
                                        100UL,
                                        g_ms_ticks);
        }
    }
}
