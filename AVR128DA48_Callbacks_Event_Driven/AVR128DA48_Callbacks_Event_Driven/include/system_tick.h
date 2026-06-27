#ifndef SYSTEM_TICK_H
#define SYSTEM_TICK_H

#include <stdint.h>

/**
 * @brief Configures TCB0 to generate a 1 ms periodic interrupt.
 */
void system_tick_init(void);

/**
 * @brief Returns elapsed milliseconds using an atomic read.
 *
 * Returns:
 *     Milliseconds elapsed since system tick initialization.
 */
uint32_t system_tick_get(void);

/**
 * @brief Returns elapsed milliseconds while already in interrupt context.
 *
 * Returns:
 *     Milliseconds elapsed since system tick initialization.
 */
uint32_t system_tick_get_from_isr(void);

/**
 * @brief Handles one TCB0 system tick interrupt.
 */
void system_tick_irq_handler(void);

#endif
