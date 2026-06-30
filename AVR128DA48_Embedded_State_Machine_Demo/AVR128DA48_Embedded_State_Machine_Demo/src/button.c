#include "button.h"

#include "hardware.h"
#include "system_time.h"

#define BUTTON_DEBOUNCE_TIME_MS 20U

typedef enum
{
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_DEBOUNCE_PRESS,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_DEBOUNCE_RELEASE
} button_state_t;

static button_state_t g_button_state = BUTTON_STATE_RELEASED;
static uint32_t g_button_deadline_ms = 0U;

/**
 * Initializes the debounced button state from the current hardware input.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void button_init(uint32_t now_ms)
{
    if (hardware_button_is_pressed())
    {
        g_button_state = BUTTON_STATE_PRESSED;
    }
    else
    {
        g_button_state = BUTTON_STATE_RELEASED;
    }

    g_button_deadline_ms = now_ms;
}

/**
 * Updates the button debounce state machine.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     True when a new debounced press event is detected; otherwise false.
 */
bool button_update(uint32_t now_ms)
{
    bool pressed = hardware_button_is_pressed();
    bool press_event = false;

    switch (g_button_state)
    {
        case BUTTON_STATE_RELEASED:
            if (pressed)
            {
                g_button_deadline_ms = now_ms + BUTTON_DEBOUNCE_TIME_MS;
                g_button_state = BUTTON_STATE_DEBOUNCE_PRESS;
            }
            break;

        case BUTTON_STATE_DEBOUNCE_PRESS:
            if (!pressed)
            {
                g_button_state = BUTTON_STATE_RELEASED;
            }
            else if (system_time_deadline_reached(now_ms,
                                                  g_button_deadline_ms))
            {
                g_button_state = BUTTON_STATE_PRESSED;
                press_event = true;
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!pressed)
            {
                g_button_deadline_ms = now_ms + BUTTON_DEBOUNCE_TIME_MS;
                g_button_state = BUTTON_STATE_DEBOUNCE_RELEASE;
            }
            break;

        case BUTTON_STATE_DEBOUNCE_RELEASE:
            if (pressed)
            {
                g_button_state = BUTTON_STATE_PRESSED;
            }
            else if (system_time_deadline_reached(now_ms,
                                                  g_button_deadline_ms))
            {
                g_button_state = BUTTON_STATE_RELEASED;
            }
            break;

        default:
            g_button_state = BUTTON_STATE_RELEASED;
            break;
    }

    return press_event;
}
