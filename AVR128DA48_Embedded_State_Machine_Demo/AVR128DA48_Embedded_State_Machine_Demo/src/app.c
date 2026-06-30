#include "app.h"

#include "hardware.h"
#include "system_time.h"
#include "uart.h"

#include <stdbool.h>
#include <stddef.h>

#define STARTING_DURATION_MS 1000U
#define STOPPING_DURATION_MS 500U
#define RUNNING_LED_PERIOD_MS 250U
#define FAULT_LED_PERIOD_MS 100U

typedef struct
{
    app_state_t state;
    app_fault_t fault;
    uint32_t state_deadline_ms;
    uint32_t led_deadline_ms;
    uint16_t last_queue_overflow_count;
    bool fault_event_latched;
} app_context_t;

static app_context_t g_app;
static event_queue_t g_event_queue;

/**
 * Converts an application state to a human-readable string.
 *
 * Args:
 *     state: Application state value.
 *
 * Returns:
 *     Pointer to a static state-name string.
 */
static const char *app_state_to_string(app_state_t state)
{
    switch (state)
    {
        case APP_STATE_IDLE:
            return "IDLE";

        case APP_STATE_STARTING:
            return "STARTING";

        case APP_STATE_RUNNING:
            return "RUNNING";

        case APP_STATE_STOPPING:
            return "STOPPING";

        case APP_STATE_FAULT:
            return "FAULT";

        default:
            return "UNKNOWN";
    }
}

/**
 * Writes one transition record to the USART diagnostic interface.
 *
 * Args:
 *     previous_state: State active before the transition.
 *     event: Event that caused the transition.
 *     next_state: State active after the transition.
 *     timestamp_ms: Transition timestamp in milliseconds.
 *
 * Returns:
 *     None.
 */
static void app_log_transition(app_state_t previous_state,
                               app_event_type_t event,
                               app_state_t next_state,
                               uint32_t timestamp_ms)
{
    uart_write_u32(timestamp_ms);
    uart_write_string(" ms: ");
    uart_write_string(app_state_to_string(previous_state));
    uart_write_string(" + event ");
    uart_write_u32((uint32_t)event);
    uart_write_string(" -> ");
    uart_write_string(app_state_to_string(next_state));
    uart_write_string("\r\n");
}

/**
 * Performs one-time hardware actions when a state becomes active.
 *
 * Args:
 *     state: State being entered.
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
static void app_enter_state(app_state_t state, uint32_t now_ms)
{
    switch (state)
    {
        case APP_STATE_IDLE:
            hardware_motor_set(false);
            hardware_status_led_set(false);
            break;

        case APP_STATE_STARTING:
            hardware_motor_set(true);
            hardware_status_led_set(true);
            g_app.state_deadline_ms = now_ms + STARTING_DURATION_MS;
            break;

        case APP_STATE_RUNNING:
            hardware_motor_set(true);
            hardware_status_led_set(true);
            g_app.led_deadline_ms = now_ms + RUNNING_LED_PERIOD_MS;
            break;

        case APP_STATE_STOPPING:
            hardware_motor_set(false);
            hardware_status_led_set(true);
            g_app.state_deadline_ms = now_ms + STOPPING_DURATION_MS;
            break;

        case APP_STATE_FAULT:
            hardware_motor_set(false);
            hardware_status_led_set(true);
            g_app.led_deadline_ms = now_ms + FAULT_LED_PERIOD_MS;
            uart_write_string("FAULT code: ");
            uart_write_u32((uint32_t)g_app.fault);
            uart_write_string("\r\n");
            break;

        default:
            // The default output condition is always safe.
            hardware_motor_set(false);
            hardware_status_led_set(true);
            break;
    }
}

/**
 * Performs one-time cleanup before leaving a state.
 *
 * Args:
 *     state: State being exited.
 *
 * Returns:
 *     None.
 */
static void app_exit_state(app_state_t state)
{
    switch (state)
    {
        case APP_STATE_STARTING:
        case APP_STATE_STOPPING:
            // Invalidate the old deadline so it cannot affect a later state.
            g_app.state_deadline_ms = 0U;
            break;

        default:
            break;
    }
}

/**
 * Moves the application to a new state through a centralized transition path.
 *
 * Args:
 *     next_state: Destination state.
 *     event: Event responsible for the transition.
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
static void app_transition(app_state_t next_state,
                           app_event_type_t event,
                           uint32_t now_ms)
{
    app_state_t previous_state = g_app.state;

    app_exit_state(previous_state);
    g_app.state = next_state;
    app_enter_state(next_state, now_ms);
    app_log_transition(previous_state, event, next_state, now_ms);
}

/**
 * Dispatches one event according to the current application state.
 *
 * Args:
 *     event: Pointer to the event being processed.
 *
 * Returns:
 *     None.
 */
static void app_dispatch_event(const app_event_t *event)
{
    if (event == NULL)
    {
        return;
    }

    switch (g_app.state)
    {
        case APP_STATE_IDLE:
            if (event->type == APP_EVENT_BUTTON_PRESSED)
            {
                app_transition(APP_STATE_STARTING,
                               event->type,
                               event->timestamp_ms);
            }
            else if (event->type == APP_EVENT_FAULT_DETECTED)
            {
                g_app.fault = APP_FAULT_EXTERNAL_INPUT;
                app_transition(APP_STATE_FAULT,
                               event->type,
                               event->timestamp_ms);
            }
            break;

        case APP_STATE_STARTING:
            if (event->type == APP_EVENT_START_TIMEOUT)
            {
                app_transition(APP_STATE_RUNNING,
                               event->type,
                               event->timestamp_ms);
            }
            else if (event->type == APP_EVENT_BUTTON_PRESSED)
            {
                app_transition(APP_STATE_STOPPING,
                               event->type,
                               event->timestamp_ms);
            }
            else if (event->type == APP_EVENT_FAULT_DETECTED)
            {
                g_app.fault = APP_FAULT_EXTERNAL_INPUT;
                app_transition(APP_STATE_FAULT,
                               event->type,
                               event->timestamp_ms);
            }
            break;

        case APP_STATE_RUNNING:
            if (event->type == APP_EVENT_BUTTON_PRESSED)
            {
                app_transition(APP_STATE_STOPPING,
                               event->type,
                               event->timestamp_ms);
            }
            else if (event->type == APP_EVENT_FAULT_DETECTED)
            {
                g_app.fault = APP_FAULT_EXTERNAL_INPUT;
                app_transition(APP_STATE_FAULT,
                               event->type,
                               event->timestamp_ms);
            }
            break;

        case APP_STATE_STOPPING:
            if (event->type == APP_EVENT_STOP_TIMEOUT)
            {
                app_transition(APP_STATE_IDLE,
                               event->type,
                               event->timestamp_ms);
            }
            else if (event->type == APP_EVENT_FAULT_DETECTED)
            {
                g_app.fault = APP_FAULT_EXTERNAL_INPUT;
                app_transition(APP_STATE_FAULT,
                               event->type,
                               event->timestamp_ms);
            }
            break;

        case APP_STATE_FAULT:
            if ((event->type == APP_EVENT_RESET_REQUEST) &&
                !hardware_fault_is_active())
            {
                g_app.fault = APP_FAULT_NONE;
                g_app.fault_event_latched = false;
                app_transition(APP_STATE_IDLE,
                               event->type,
                               event->timestamp_ms);
            }
            break;

        default:
            g_app.fault = APP_FAULT_INVALID_STATE;
            g_app.state = APP_STATE_FAULT;
            app_enter_state(APP_STATE_FAULT, event->timestamp_ms);
            break;
    }
}

/**
 * Initializes the application state machine and its event queue.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void app_init(uint32_t now_ms)
{
    event_queue_init(&g_event_queue);

    g_app.state = APP_STATE_IDLE;
    g_app.fault = APP_FAULT_NONE;
    g_app.state_deadline_ms = 0U;
    g_app.led_deadline_ms = 0U;
    g_app.last_queue_overflow_count = 0U;
    g_app.fault_event_latched = false;

    app_enter_state(APP_STATE_IDLE, now_ms);
    uart_write_string("AVR128DA48 event-driven state machine ready.\r\n");
}

/**
 * Updates time-driven application behavior without blocking.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void app_update(uint32_t now_ms)
{
    uint16_t overflow_count = event_queue_get_overflow_count(&g_event_queue);

    if (overflow_count != g_app.last_queue_overflow_count)
    {
        g_app.last_queue_overflow_count = overflow_count;
        g_app.fault = APP_FAULT_EVENT_QUEUE_OVERFLOW;
        app_transition(APP_STATE_FAULT,
                       APP_EVENT_FAULT_DETECTED,
                       now_ms);
    }

    if (hardware_fault_is_active())
    {
        if (!g_app.fault_event_latched)
        {
            g_app.fault_event_latched = true;
            app_post_event(APP_EVENT_FAULT_DETECTED, now_ms, 0U);
        }
    }
    else
    {
        g_app.fault_event_latched = false;
    }

    switch (g_app.state)
    {
        case APP_STATE_STARTING:
            if (system_time_deadline_reached(now_ms,
                                             g_app.state_deadline_ms))
            {
                g_app.state_deadline_ms = 0U;
                app_post_event(APP_EVENT_START_TIMEOUT, now_ms, 0U);
            }
            break;

        case APP_STATE_RUNNING:
            if (system_time_deadline_reached(now_ms,
                                             g_app.led_deadline_ms))
            {
                hardware_status_led_toggle();
                g_app.led_deadline_ms = now_ms + RUNNING_LED_PERIOD_MS;
            }
            break;

        case APP_STATE_STOPPING:
            if (system_time_deadline_reached(now_ms,
                                             g_app.state_deadline_ms))
            {
                g_app.state_deadline_ms = 0U;
                app_post_event(APP_EVENT_STOP_TIMEOUT, now_ms, 0U);
            }
            break;

        case APP_STATE_FAULT:
            if (system_time_deadline_reached(now_ms,
                                             g_app.led_deadline_ms))
            {
                hardware_status_led_toggle();
                g_app.led_deadline_ms = now_ms + FAULT_LED_PERIOD_MS;
            }
            break;

        default:
            break;
    }
}

/**
 * Posts an application event to the shared event queue.
 *
 * Args:
 *     type: Event type to enqueue.
 *     timestamp_ms: Event timestamp in milliseconds.
 *     parameter: Optional event-specific parameter.
 *
 * Returns:
 *     None.
 */
void app_post_event(app_event_type_t type,
                    uint32_t timestamp_ms,
                    uint16_t parameter)
{
    app_event_t event;

    event.type = type;
    event.timestamp_ms = timestamp_ms;
    event.parameter = parameter;

    (void)event_queue_push(&g_event_queue, &event);
}

/**
 * Processes all pending application events.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void app_process_events(void)
{
    app_event_t event;

    while (event_queue_pop(&g_event_queue, &event))
    {
        // In the fault state, a button press becomes a reset request.
        if ((g_app.state == APP_STATE_FAULT) &&
            (event.type == APP_EVENT_BUTTON_PRESSED))
        {
            event.type = APP_EVENT_RESET_REQUEST;
        }

        app_dispatch_event(&event);
    }
}

/**
 * Gets the current application state.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current application state.
 */
app_state_t app_get_state(void)
{
    return g_app.state;
}
