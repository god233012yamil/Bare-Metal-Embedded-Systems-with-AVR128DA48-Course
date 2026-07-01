#include "soft_timer.h"

/**
 * Initializes a cooperative software timer.
 *
 * Args:
 *     timer: Timer object to initialize.
 *     period_ms: Timer period in milliseconds.
 *     callback: Function called when the timer expires.
 *     context: User context passed to the callback.
 */
void soft_timer_init(soft_timer_t *timer,
                     uint16_t period_ms,
                     soft_timer_callback_t callback,
                     void *context)
{
    if (timer == 0)
    {
        return;
    }

    timer->period_ms = period_ms;
    timer->elapsed_ms = 0U;
    timer->callback = callback;
    timer->context = context;
    timer->enabled = 0U;
}

/**
 * Starts a software timer from zero.
 *
 * Args:
 *     timer: Timer object to start.
 */
void soft_timer_start(soft_timer_t *timer)
{
    if (timer == 0)
    {
        return;
    }

    timer->elapsed_ms = 0U;
    timer->enabled = 1U;
}

/**
 * Stops a software timer.
 *
 * Args:
 *     timer: Timer object to stop.
 */
void soft_timer_stop(soft_timer_t *timer)
{
    if (timer == 0)
    {
        return;
    }

    timer->enabled = 0U;
}

/**
 * Advances a software timer by one millisecond.
 *
 * Args:
 *     timer: Timer object to update.
 */
void soft_timer_tick(soft_timer_t *timer)
{
    if ((timer == 0) || (timer->enabled == 0U))
    {
        return;
    }

    timer->elapsed_ms++;

    if (timer->elapsed_ms >= timer->period_ms)
    {
        timer->elapsed_ms = 0U;

        if (timer->callback != 0)
        {
            timer->callback(timer->context);
        }
    }
}
