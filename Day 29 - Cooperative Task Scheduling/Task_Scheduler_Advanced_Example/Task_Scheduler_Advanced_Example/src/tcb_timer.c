#include "tcb_timer.h"
#include <avr/io.h>

void tcb0_init(void)
{
    /* Set TOP value for 1 ms period */
    TCB0.CCMP = TCB0_CCMP_VALUE;

    /* Periodic Interrupt Mode, CLK_PER/2 prescaler */
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;

    /* Enable capture/compare interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Clear any pending flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;
}
