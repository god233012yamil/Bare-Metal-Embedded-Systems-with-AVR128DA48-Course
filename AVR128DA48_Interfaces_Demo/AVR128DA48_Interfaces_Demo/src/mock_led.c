#include "mock_led.h"

/**
 * Sets the state of a mock LED object.
 *
 * Args:
 *   context: Pointer to a mock_led_context_t instance.
 *   state: Requested LED state.
 */
static void mock_led_set(void *context, bool state)
{
    mock_led_context_t *led = (mock_led_context_t *)context;

    led->state = state;
}

/**
 * Toggles the state of a mock LED object.
 *
 * Args:
 *   context: Pointer to a mock_led_context_t instance.
 */
static void mock_led_toggle(void *context)
{
    mock_led_context_t *led = (mock_led_context_t *)context;

    led->state = !led->state;
    led->toggle_count++;
}

/**
 * Creates a mock LED implementation of the generic LED interface.
 *
 * Args:
 *   interface: Generic LED interface to initialize.
 *   context: Mock LED private state storage.
 */
void mock_led_create(led_interface_t *interface, mock_led_context_t *context)
{
    context->state = false;
    context->toggle_count = 0U;

    interface->context = context;
    interface->set = mock_led_set;
    interface->toggle = mock_led_toggle;
}
