#include "software_timer.h"

#include <stddef.h>
#include <util/atomic.h>

static volatile uint32_t remaining_ms;
static volatile bool timer_running;
static software_timer_callback_t timer_callback;
static void *timer_context;

/**
 * @brief Initializes the one-shot software timer.
 */
void software_timer_init(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        remaining_ms = 0U;
        timer_running = false;
        timer_callback = NULL;
        timer_context = NULL;
    }
}

/**
 * @brief Starts or restarts the one-shot software timer.
 *
 * Args:
 *     timeout_ms: Timeout in milliseconds. A value of zero expires on the next
 *         system tick.
 *     callback: Function called from the timer ISR when the timeout expires.
 *     context: Application-owned pointer passed back to the callback.
 *
 * Returns:
 *     true when the callback is valid and the timer starts, otherwise false.
 */
bool software_timer_start(uint32_t timeout_ms,
                          software_timer_callback_t callback,
                          void *context)
{
    if (callback == NULL)
    {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        remaining_ms = timeout_ms;
        timer_callback = callback;
        timer_context = context;
        timer_running = true;
    }

    return true;
}

/**
 * @brief Stops the software timer without invoking its callback.
 */
void software_timer_stop(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        timer_running = false;
        remaining_ms = 0U;
    }
}

/**
 * @brief Advances the software timer by one millisecond.
 *
 * Notes:
 *     This function is called from the 1 ms timer interrupt.
 */
void software_timer_tick_1ms(void)
{
    software_timer_callback_t callback;
    void *context;

    if (!timer_running)
    {
        return;
    }

    if (remaining_ms > 0U)
    {
        remaining_ms--;
    }

    if (remaining_ms != 0U)
    {
        return;
    }

    timer_running = false;
    callback = timer_callback;
    context = timer_context;

    if (callback != NULL)
    {
        callback(context);
    }
}
