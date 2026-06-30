#include "event_queue.h"

#include <stddef.h>
#include <util/atomic.h>

/**
 * Calculates the next circular-buffer index.
 *
 * Args:
 *     index: Current circular-buffer index.
 *
 * Returns:
 *     Next valid index in the event queue.
 */
static uint8_t event_queue_next_index(uint8_t index)
{
    return (uint8_t)((index + 1U) % EVENT_QUEUE_CAPACITY);
}

/**
 * Initializes an event queue to the empty state.
 *
 * Args:
 *     queue: Event queue to initialize.
 *
 * Returns:
 *     None.
 */
void event_queue_init(event_queue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    queue->head = 0U;
    queue->tail = 0U;
    queue->overflow_count = 0U;
}

/**
 * Pushes an event into the queue if space is available.
 *
 * Args:
 *     queue: Event queue that receives the event.
 *     event: Event to copy into the queue.
 *
 * Returns:
 *     True if the event was queued; otherwise false.
 */
bool event_queue_push(event_queue_t *queue, const app_event_t *event)
{
    uint8_t next_head;
    bool pushed = false;

    if ((queue == NULL) || (event == NULL))
    {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        next_head = event_queue_next_index(queue->head);

        if (next_head != queue->tail)
        {
            queue->events[queue->head] = *event;
            queue->head = next_head;
            pushed = true;
        }
        else
        {
            // Saturate the diagnostic counter instead of allowing wraparound.
            if (queue->overflow_count < UINT16_MAX)
            {
                queue->overflow_count++;
            }
        }
    }

    return pushed;
}

/**
 * Pops the oldest event from the queue.
 *
 * Args:
 *     queue: Event queue to read from.
 *     event: Destination for the popped event.
 *
 * Returns:
 *     True if an event was available; otherwise false.
 */
bool event_queue_pop(event_queue_t *queue, app_event_t *event)
{
    bool popped = false;

    if ((queue == NULL) || (event == NULL))
    {
        return false;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (queue->tail != queue->head)
        {
            *event = queue->events[queue->tail];
            queue->tail = event_queue_next_index(queue->tail);
            popped = true;
        }
    }

    return popped;
}

/**
 * Gets the number of failed event pushes caused by a full queue.
 *
 * Args:
 *     queue: Event queue to inspect.
 *
 * Returns:
 *     Saturating queue overflow count.
 */
uint16_t event_queue_get_overflow_count(const event_queue_t *queue)
{
    uint16_t count = 0U;

    if (queue == NULL)
    {
        return 0U;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        count = queue->overflow_count;
    }

    return count;
}
