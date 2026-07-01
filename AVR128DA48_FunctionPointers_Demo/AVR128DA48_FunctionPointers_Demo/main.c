#include <avr/io.h>
#include <avr/interrupt.h>
#include "app_events.h"
#include "board.h"

/**
 * Initializes TCA0 to generate a 1 ms periodic interrupt.
 */
static void system_tick_init(void)
{
    TCA0.SINGLE.CTRLA = 0U;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
    TCA0.SINGLE.PER = 499U;
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV8_gc | TCA_SINGLE_ENABLE_bm;
}

/**
 * Handles the TCA0 overflow interrupt used as the cooperative system tick.
 */
ISR(TCA0_OVF_vect)
{
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    app_request_tick();
}

/**
 * Main application entry point.
 *
 * Returns:
 *     This function does not return.
 */
int main(void)
{
    board_init();
    system_tick_init();
    app_init();

    while (1)
    {
        app_process();
    }
}
