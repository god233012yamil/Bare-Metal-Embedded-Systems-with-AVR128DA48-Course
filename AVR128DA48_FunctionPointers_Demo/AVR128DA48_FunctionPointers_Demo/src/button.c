#include "button.h"

/**
 * Initializes a debounced button object.
 *
 * Args:
 *     button: Button object to initialize.
 *     read: Function used to read the hardware button state.
 *     callback: Function called when a debounced button event occurs.
 *     context: User context passed back to the callback.
 *     debounce_ticks: Number of stable samples required before an event is generated.
 */
void button_init(button_t *button,
                 button_read_fn_t read,
                 button_event_callback_t callback,
                 void *context,
                 uint8_t debounce_ticks)
{
    if (button == 0)
    {
        return;
    }

    button->read = read;
    button->callback = callback;
    button->context = context;
    button->stable_state = 0U;
    button->last_sample = 0U;
    button->debounce_count = 0U;
    button->debounce_ticks = debounce_ticks;
}

/**
 * Processes one debouncing sample and emits button events through the callback.
 *
 * Args:
 *     button: Button object to process.
 */
void button_process(button_t *button)
{
    uint8_t sample;

    if ((button == 0) || (button->read == 0))
    {
        return;
    }

    sample = button->read();

    if (sample == button->last_sample)
    {
        if (button->debounce_count < button->debounce_ticks)
        {
            button->debounce_count++;
        }
    }
    else
    {
        button->debounce_count = 0U;
        button->last_sample = sample;
    }

    if ((button->debounce_count >= button->debounce_ticks) &&
        (sample != button->stable_state))
    {
        button->stable_state = sample;

        if (button->callback != 0)
        {
            button->callback((sample != 0U) ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED,
                             button->context);
        }
    }
}
