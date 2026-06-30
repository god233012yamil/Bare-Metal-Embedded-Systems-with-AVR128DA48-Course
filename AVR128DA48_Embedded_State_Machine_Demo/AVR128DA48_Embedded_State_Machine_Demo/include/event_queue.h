#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

#define EVENT_QUEUE_CAPACITY 16U

typedef enum
{
    APP_EVENT_NONE = 0,
    APP_EVENT_BUTTON_PRESSED,
    APP_EVENT_START_TIMEOUT,
    APP_EVENT_STOP_TIMEOUT,
    APP_EVENT_FAULT_DETECTED,
    APP_EVENT_RESET_REQUEST
} app_event_type_t;

typedef struct
{
    app_event_type_t type;
    uint32_t timestamp_ms;
    uint16_t parameter;
} app_event_t;

typedef struct
{
    app_event_t events[EVENT_QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint16_t overflow_count;
} event_queue_t;

/**
 * Initializes an event queue.
 *
 * Args:
 *     queue: Pointer to the event queue instance.
 *
 * Returns:
 *     None.
 */
void event_queue_init(event_queue_t *queue);

/**
 * Adds an event to the queue.
 *
 * Args:
 *     queue: Pointer to the event queue instance.
 *     event: Pointer to the event to copy into the queue.
 *
 * Returns:
 *     true when the event was queued, otherwise false when the queue was full.
 */
bool event_queue_push(event_queue_t *queue, const app_event_t *event);

/**
 * Removes the oldest event from the queue.
 *
 * Args:
 *     queue: Pointer to the event queue instance.
 *     event: Pointer that receives the removed event.
 *
 * Returns:
 *     true when an event was returned, otherwise false when the queue was empty.
 */
bool event_queue_pop(event_queue_t *queue, app_event_t *event);

/**
 * Returns the number of events lost because the queue was full.
 *
 * Args:
 *     queue: Pointer to the event queue instance.
 *
 * Returns:
 *     Queue overflow counter value.
 */
uint16_t event_queue_get_overflow_count(const event_queue_t *queue);

#endif
