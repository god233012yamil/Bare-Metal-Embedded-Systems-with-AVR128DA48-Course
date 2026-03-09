#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#include "twi0.h"

/*
File: main.c

AVR128DA48 Curiosity Nano - TWI0 (I2C) demo project (blocking).

What this project demonstrates:
- TWI0 Host mode init at 100 kHz
- Read register pattern used by many sensors:
    write register address
    repeated START
    read one byte
- Simple success/fail indication using the on-board LED (PC6, active-low)

Wiring notes:
- Default TWI0 pins on AVR128DA48 Curiosity Nano:
    SDA: PA2
    SCL: PA3
- External pull-ups are required for reliable I2C.

How to test:
- Connect any I2C device to PA2/PA3 and GND/VCC.
- Update DEMO_DEV_ADDR and DEMO_REG_ADDR for your device.
- If the transaction succeeds, LED blinks slowly.
- If it fails (NACK), LED blinks fast.
*/

#ifndef F_CPU
#define F_CPU 4000000UL
#endif

#define I2C_BAUD_HZ     100000UL

/* Change these to match your I2C device. */
#define DEMO_DEV_ADDR   0x50u
#define DEMO_REG_ADDR   0x00u

/* LED0 on Curiosity Nano is on PC6 and is active-low. */
#define LED_PORT        PORTC
#define LED_PIN_bm      PIN6_bm

/*
Initialize the on-board LED pin (PC6) as output.

Args:
    None.

Returns:
    None.
*/
static void led_init(void)
{
    /* Configure PC6 as output. */
    LED_PORT.DIRSET = LED_PIN_bm;

    /* Turn LED off (active-low). */
    LED_PORT.OUTSET = LED_PIN_bm;
}

/*
Turn the on-board LED on (active-low).

Args:
    None.

Returns:
    None.
*/
static void led_on(void)
{
    LED_PORT.OUTCLR = LED_PIN_bm;
}

/*
Turn the on-board LED off (active-low).

Args:
    None.

Returns:
    None.
*/
static void led_off(void)
{
    LED_PORT.OUTSET = LED_PIN_bm;
}

/*
Blink the LED a given number of times with a fixed delay.

Args:
    count: Number of blinks.
    delay_ms: Delay per half-period in milliseconds.

Returns:
    None.
*/
static void led_blink(uint8_t count, uint16_t delay_ms)
{
    uint8_t i;

    for (i = 0; i < count; i++)
    {
        led_on();
        _delay_ms(delay_ms);

        led_off();
        _delay_ms(delay_ms);
    }
}

/*
Application entry point.

Args:
    None.

Returns:
    Does not return.
*/
int main(void)
{
    uint8_t reg_value = 0;
    bool ok;

    led_init();
    twi0_init(I2C_BAUD_HZ);

    while (1)
    {
        /* Attempt a classic "read register" transaction. */
        ok = twi0_read_reg(DEMO_DEV_ADDR, DEMO_REG_ADDR, &reg_value);

        if (ok)
        {
            /* Success: slow blink (1 blink). */
            led_blink(1, 200);
            _delay_ms(800);
        }
        else
        {
            /* Failure: fast blink (3 blinks). */
            led_blink(3, 80);
            _delay_ms(500);
        }
    }
}
