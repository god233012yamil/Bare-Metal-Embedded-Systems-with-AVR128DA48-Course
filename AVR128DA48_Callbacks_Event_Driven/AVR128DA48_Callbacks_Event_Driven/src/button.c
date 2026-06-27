#include "button.h"

#include "board.h"
#include "system_tick.h"

#include <avr/io.h>
#include <stddef.h>

#define BUTTON_ID_USER 0U
#define BUTTON_DEBOUNCE_MS 50UL

static button_callback_t registered_callback;
static void *registered_context;
static uint32_t last_accepted_press_ms;

/**
 * @brief Initializes callback storage for the button driver.
 */
void button_init(void)
{
    registered_callback = NULL;
    registered_context = NULL;
    last_accepted_press_ms = 0U;
}

/**
 * @brief Registers the function called when the button interrupt is accepted.
 *
 * Args:
 *     callback: Function invoked from interrupt context. Pass NULL to disable
 *         event notification.
 *     context: Application-owned pointer passed back to the callback.
 */
void button_register_callback(button_callback_t callback, void *context)
{
    registered_callback = callback;
    registered_context = context;
}

/**
 * @brief Processes the hardware button interrupt.
 *
 * Notes:
 *     This function executes from interrupt context and must remain short.
 */
void button_irq_handler(void)
{
    const uint32_t now_ms = system_tick_get_from_isr();
    const uint32_t elapsed_ms = now_ms - last_accepted_press_ms;

    if (!board_button_is_pressed())
    {
        return;
    }

    if ((last_accepted_press_ms != 0U) &&
        (elapsed_ms < BUTTON_DEBOUNCE_MS))
    {
        return;
    }

    last_accepted_press_ms = now_ms;

    if (registered_callback != NULL)
    {
        registered_callback(BUTTON_ID_USER, registered_context);
    }
}
