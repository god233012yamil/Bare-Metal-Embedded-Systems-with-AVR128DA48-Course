#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_RELEASED = 0,
    BUTTON_EVENT_PRESSED
} button_event_t;

typedef uint8_t (*button_read_fn_t)(void);
typedef void (*button_event_callback_t)(button_event_t event, void *context);

typedef struct
{
    button_read_fn_t read;
    button_event_callback_t callback;
    void *context;
    uint8_t stable_state;
    uint8_t last_sample;
    uint8_t debounce_count;
    uint8_t debounce_ticks;
} button_t;

void button_init(button_t *button,
                 button_read_fn_t read,
                 button_event_callback_t callback,
                 void *context,
                 uint8_t debounce_ticks);

void button_process(button_t *button);

#endif
