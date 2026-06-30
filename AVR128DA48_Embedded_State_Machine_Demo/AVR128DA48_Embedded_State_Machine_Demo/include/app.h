#ifndef APP_H
#define APP_H

#include "event_queue.h"

#include <stdint.h>

typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_STARTING,
    APP_STATE_RUNNING,
    APP_STATE_STOPPING,
    APP_STATE_FAULT
} app_state_t;

typedef enum
{
    APP_FAULT_NONE = 0,
    APP_FAULT_EXTERNAL_INPUT,
    APP_FAULT_EVENT_QUEUE_OVERFLOW,
    APP_FAULT_INVALID_STATE
} app_fault_t;

/**
 * Initializes the application state machine and event queue.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void app_init(uint32_t now_ms);

/**
 * Executes periodic state-machine work and generates timeout events.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void app_update(uint32_t now_ms);

/**
 * Queues an application event for deterministic processing.
 *
 * Args:
 *     type: Event type to queue.
 *     timestamp_ms: Time at which the event was generated.
 *     parameter: Optional event-specific value.
 *
 * Returns:
 *     None.
 */
void app_post_event(app_event_type_t type,
                    uint32_t timestamp_ms,
                    uint16_t parameter);

/**
 * Processes all currently queued events.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void app_process_events(void);

/**
 * Returns the current application state.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current application state value.
 */
app_state_t app_get_state(void);

#endif
