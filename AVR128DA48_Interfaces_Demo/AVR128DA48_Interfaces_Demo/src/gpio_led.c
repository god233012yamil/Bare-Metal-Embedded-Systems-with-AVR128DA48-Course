#include "gpio_led.h"

#include <stdbool.h>

/**
 * Sets the state of a GPIO-connected LED.
 *
 * Args:
 *   context: Pointer to a gpio_led_context_t instance.
 *   state: true turns the LED on, false turns it off.
 */
static void gpio_led_set(void *context, bool state)
{
    gpio_led_context_t *led = (gpio_led_context_t *)context;

    if (state) {
        led->port->OUTSET = led->pin_mask;
    } else {
        led->port->OUTCLR = led->pin_mask;
    }
}

/**
 * Toggles a GPIO-connected LED.
 *
 * Args:
 *   context: Pointer to a gpio_led_context_t instance.
 */
static void gpio_led_toggle(void *context)
{
    gpio_led_context_t *led = (gpio_led_context_t *)context;

    led->port->OUTTGL = led->pin_mask;
}

/**
 * Creates a GPIO LED implementation of the generic LED interface.
 *
 * Args:
 *   interface: Generic LED interface to initialize.
 *   context: GPIO LED private state storage.
 *   port: AVR Dx PORT peripheral used by the LED.
 *   pin_mask: Bit mask for the LED pin.
 */
void gpio_led_create(led_interface_t *interface,
                     gpio_led_context_t *context,
                     PORT_t *port,
                     uint8_t pin_mask)
{
    context->port = port;
    context->pin_mask = pin_mask;

    interface->context = context;
    interface->set = gpio_led_set;
    interface->toggle = gpio_led_toggle;
}
