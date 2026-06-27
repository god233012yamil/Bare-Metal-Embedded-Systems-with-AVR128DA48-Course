#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

typedef void (*button_callback_t)(uint8_t button_id, void *context);

/**
 * @brief Initializes callback storage for the button driver.
 */
void button_init(void);

/**
 * @brief Registers the function called when the button interrupt is accepted.
 *
 * Args:
 *     callback: Function invoked from interrupt context. Pass NULL to disable
 *         event notification.
 *     context: Application-owned pointer passed back to the callback.
 */
void button_register_callback(button_callback_t callback, void *context);

/**
 * @brief Processes the hardware button interrupt.
 *
 * Notes:
 *     This function executes from interrupt context and must remain short.
 */
void button_irq_handler(void);

#endif
