#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_EVENT_NONE = 0,
    APP_EVENT_BUTTON_PRESSED,
    APP_EVENT_TIMEOUT
} app_event_type_t;

typedef struct
{
    app_event_type_t type;
    uint32_t data;
} app_event_t;

/**
 * @brief Initializes the application event queue.
 */
void app_event_init(void);

/**
 * @brief Adds an event to the queue from interrupt context.
 *
 * Args:
 *     event: Event to add to the queue.
 *
 * Returns:
 *     true if the event was queued, otherwise false when the queue is full.
 */
bool app_event_push_from_isr(app_event_t event);

/**
 * @brief Removes the oldest event from the queue.
 *
 * Args:
 *     event: Destination for the dequeued event.
 *
 * Returns:
 *     true if an event was returned, otherwise false when the queue is empty
 *     or the destination pointer is NULL.
 */
bool app_event_pop(app_event_t *event);

/**
 * @brief Returns the number of events dropped because the queue was full.
 *
 * Returns:
 *     Number of dropped events since initialization.
 */
uint16_t app_event_get_dropped_count(void);

#endif
