#include "led_driver.h"

static const led_interface_t *active_led = 0;

/**
 * Initializes the generic LED driver with a hardware interface.
 *
 * Args:
 *     led: Pointer to an LED interface containing write and toggle functions.
 */
void led_driver_init(const led_interface_t *led)
{
    active_led = led;
}

/**
 * Sets the configured LED output state.
 *
 * Args:
 *     state: Non-zero turns the LED on. Zero turns the LED off.
 */
void led_driver_set(uint8_t state)
{
    if ((active_led != 0) && (active_led->write != 0))
    {
        active_led->write(state);
    }
}

/**
 * Toggles the configured LED output.
 */
void led_driver_toggle(void)
{
    if ((active_led != 0) && (active_led->toggle != 0))
    {
        active_led->toggle();
    }
}
