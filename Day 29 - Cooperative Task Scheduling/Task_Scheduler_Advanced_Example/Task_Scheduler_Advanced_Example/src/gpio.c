#include "gpio.h"

void gpio_init(void)
{
    /* LED: output, drive HIGH (LED off - active LOW) */
    LED_PORT.DIRSET  = LED_PIN;
    LED_PORT.OUTSET  = LED_PIN;

    /* Button: input with pull-up */
    BTN_PORT.DIRCLR  = BTN_PIN;
    BTN_PORT.PIN2CTRL = PORT_PULLUPEN_bm;
}
