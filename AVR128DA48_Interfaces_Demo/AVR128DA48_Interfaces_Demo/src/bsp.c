#include "bsp.h"

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>

/**
 * Initializes the board support package.
 *
 * Configures the AVR128DA48 main clock to use the 4 MHz internal oscillator
 * and prepares the demo GPIO pins used by the application.
 */
void bsp_init(void)
{
    ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_4M_gc);

    PORTA.DIRSET = PIN0_bm;
    PORTA.OUTCLR = PIN0_bm;
}

/**
 * Blocks execution for the requested number of milliseconds.
 *
 * Args:
 *   delay_ms: Delay duration in milliseconds.
 */
void bsp_delay_ms(uint16_t delay_ms)
{
    while (delay_ms > 0U) {
        _delay_ms(1);
        delay_ms--;
    }
}
