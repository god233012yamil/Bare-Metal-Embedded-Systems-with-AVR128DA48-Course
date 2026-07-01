#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdint.h>

typedef void (*soft_timer_callback_t)(void *context);

typedef struct
{
    uint16_t period_ms;
    uint16_t elapsed_ms;
    soft_timer_callback_t callback;
    void *context;
    uint8_t enabled;
} soft_timer_t;

void soft_timer_init(soft_timer_t *timer,
                     uint16_t period_ms,
                     soft_timer_callback_t callback,
                     void *context);

void soft_timer_start(soft_timer_t *timer);
void soft_timer_stop(soft_timer_t *timer);
void soft_timer_tick(soft_timer_t *timer);

#endif
