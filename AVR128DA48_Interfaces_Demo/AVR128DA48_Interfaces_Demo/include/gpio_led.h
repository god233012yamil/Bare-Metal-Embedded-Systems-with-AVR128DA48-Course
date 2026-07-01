#ifndef GPIO_LED_H
#define GPIO_LED_H

#include <avr/io.h>
#include <stdint.h>
#include "interfaces.h"

typedef struct {
    PORT_t *port;
    uint8_t pin_mask;
} gpio_led_context_t;

void gpio_led_create(led_interface_t *interface,
                     gpio_led_context_t *context,
                     PORT_t *port,
                     uint8_t pin_mask);

#endif
