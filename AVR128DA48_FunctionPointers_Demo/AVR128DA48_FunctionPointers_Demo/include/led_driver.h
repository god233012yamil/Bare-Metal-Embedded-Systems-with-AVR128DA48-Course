#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

typedef void (*led_write_fn_t)(uint8_t state);
typedef void (*led_toggle_fn_t)(void);

typedef struct
{
    led_write_fn_t write;
    led_toggle_fn_t toggle;
} led_interface_t;

void led_driver_init(const led_interface_t *led);
void led_driver_set(uint8_t state);
void led_driver_toggle(void);

#endif
