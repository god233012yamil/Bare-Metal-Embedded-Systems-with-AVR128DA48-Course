#ifndef MOCK_LED_H
#define MOCK_LED_H

#include <stdbool.h>
#include <stdint.h>
#include "interfaces.h"

typedef struct {
    bool state;
    uint16_t toggle_count;
} mock_led_context_t;

void mock_led_create(led_interface_t *interface, mock_led_context_t *context);

#endif
