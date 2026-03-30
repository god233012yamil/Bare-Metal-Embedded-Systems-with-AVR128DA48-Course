#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

/*
 * Curiosity Nano pin assignments:
 *   LED0  -> PB3 (active LOW - drive LOW to turn on)
 *   SW0   -> PB2 (active LOW with internal pull-up, press = LOW)
 */

#define LED_PORT    PORTB
#define LED_PIN     PIN3_bm

#define BTN_PORT    PORTB
#define BTN_PIN     PIN2_bm

void gpio_init(void);

static inline void led_set(bool on)
{
    if (on)
        LED_PORT.OUTCLR = LED_PIN;   /* active LOW */
    else
        LED_PORT.OUTSET = LED_PIN;
}

static inline void led_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN;
}

static inline bool button_is_pressed(void)
{
    /* Button active LOW - pressed when pin reads 0 */
    return !(BTN_PORT.IN & BTN_PIN);
}

#endif /* GPIO_H */
