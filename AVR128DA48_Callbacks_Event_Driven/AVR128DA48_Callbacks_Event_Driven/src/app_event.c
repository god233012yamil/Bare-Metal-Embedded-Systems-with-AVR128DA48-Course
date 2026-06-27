#include "app_event.h"

#include <avr/interrupt.h>
#include <stddef.h>
#include <util/atomic.h>

#define APP_EVENT_QUEUE_CAPACITY 16U

static volatile app_event_t event_queue[APP_EVENT_QUEUE_CAPACITY];
static volatile uint8_t event_head;
static volatile uint8_t event_tail;
static volatile uint16_t dropped_event_count;

/**
 * @brief Calculates the next ring-buffer index.
 *
 * Args:
 *     index: Current ring-buffer index.
 *
 * Returns:
 *     Next index with wraparound.
 */
static uint8_t app_event_next_index(uint8_t index)
{
    index++;

    if (index >= APP_EVENT_QUEUE_CAPACITY)
    {
        index = 0U;
    }

    return index;
}

/**
 * @brief Initializes the application event queue.
 */
void app_event_init(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        event_head = 0U;
        event_tail = 0U;
        dropped_event_count = 0U;
    }
}

/**
 * @brief Adds an event to the queue from interrupt context.
 *
 * Args:
 *     event: Event to add to the queue.
 *
 * Returns:
 *     true if the event was queued, otherwise false when the queue is full.
 */
bool app_event_push_from_isr(app_event_t event)
{
    const uint8_t next_head = app_event_next_index(event_head);

    if (next_head == event_tail)
    {
        dropped_event_count++;
        return false;
    }

    event_queue[event_head] = event;
    event_head = next_head;

    return true;
}

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
bool app_event_pop(app_event_t *event)
{
    bool event_available = false;

    if (event == NULL)
    {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (event_head != event_tail)
        {
            *event = event_queue[event_tail];
            event_tail = app_event_next_index(event_tail);
            event_available = true;
        }
    }

    return event_available;
}

/**
 * @brief Returns the number of events dropped because the queue was full.
 *
 * Returns:
 *     Number of dropped events since initialization.
 */
uint16_t app_event_get_dropped_count(void)
{
    uint16_t count;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        count = dropped_event_count;
    }

    return count;
}
