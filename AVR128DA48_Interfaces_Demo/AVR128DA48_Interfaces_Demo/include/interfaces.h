#ifndef INTERFACES_H
#define INTERFACES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *context;
    void (*set)(void *context, bool state);
    void (*toggle)(void *context);
} led_interface_t;

typedef struct {
    void *context;
    int (*write)(void *context, const uint8_t *data, size_t length);
    int (*read)(void *context, uint8_t *data, size_t length);
} comm_interface_t;

#endif
