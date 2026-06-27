#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*software_timer_callback_t)(void *context);

/**
 * @brief Initializes the one-shot software timer.
 */
void software_timer_init(void);

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
                          void *context);

/**
 * @brief Stops the software timer without invoking its callback.
 */
void software_timer_stop(void);

/**
 * @brief Advances the software timer by one millisecond.
 *
 * Notes:
 *     This function is called from the 1 ms timer interrupt.
 */
void software_timer_tick_1ms(void);

#endif
