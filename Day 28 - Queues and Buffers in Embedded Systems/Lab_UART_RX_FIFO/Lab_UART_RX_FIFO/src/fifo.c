/**
 * @file fifo.c
 * @brief FIFO circular buffer implementation for UART receive buffering.
 *
 * Implements all functions declared in fifo.h using a lock-free, single-producer
 * / single-consumer circular buffer pattern. The design relies on:
 *   - head being written only by the ISR (producer).
 *   - tail being written only by the main loop (consumer).
 *   - Atomic 8-bit reads/writes on AVR (no additional critical sections needed
 *     for index updates themselves, because AVR index variables are uint8_t).
 *
 * The power-of-two buffer size allows index wrapping via bitwise AND, avoiding
 * a potentially expensive modulo operation.
 *
 * @note If FIFO_BUFFER_SIZE is changed it must remain a power of two, otherwise
 *       FIFO_BUFFER_MASK will not produce correct wrap-around behaviour.
 */

#include "fifo.h"

/* =========================================================================
 * Public API implementation
 * ====================================================================== */

/**
 * @brief Initialise a FIFO by zeroing both indices.
 *
 * Safe to call at any time; after the call the buffer appears empty. Any data
 * that was present before the call is logically discarded (the backing array
 * bytes are not overwritten, but they will be overwritten before they are read
 * again because head == tail after reset).
 *
 * @param fifo  Non-NULL pointer to the FIFO_t to initialise.
 */
void FIFO_Init(FIFO_t *fifo)
{
    fifo->head = 0u;
    fifo->tail = 0u;
}

/**
 * @brief Write one byte into the FIFO (ISR-safe producer).
 *
 * Computes the next head position before writing so that the new head value is
 * only committed after the byte has been stored.  This ensures the consumer
 * never sees an index pointing to a slot that has not yet been written.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Byte to store.
 * @return true  Stored successfully.
 * @return false Buffer full; byte silently discarded.
 */
bool FIFO_Put(FIFO_t *fifo, uint8_t data)
{
    /* Calculate where head will move after this write. */
    uint8_t next_head = (uint8_t)((fifo->head + 1u) & FIFO_BUFFER_MASK);

    /* Reject the byte if advancing head would collide with tail (full). */
    if (next_head == fifo->tail)
    {
        return false;   /* Buffer overflow – byte discarded. */
    }

    /* Store the byte at the CURRENT head position, then advance head. */
    fifo->buffer[fifo->head] = data;
    fifo->head = next_head;

    return true;
}

/**
 * @brief Read and remove one byte from the FIFO (main-loop consumer).
 *
 * Reads from the current tail position and advances tail. The byte is fetched
 * before tail is updated so the ISR cannot overwrite the slot mid-read.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Output pointer; receives the retrieved byte on success.
 * @return true  A byte was available; *data is valid.
 * @return false Buffer empty; *data is not modified.
 */
bool FIFO_Get(FIFO_t *fifo, uint8_t *data)
{
    /* Nothing to read if head == tail (empty). */
    if (fifo->head == fifo->tail)
    {
        return false;
    }

    /* Retrieve byte, then advance tail. */
    *data = fifo->buffer[fifo->tail];
    fifo->tail = (uint8_t)((fifo->tail + 1u) & FIFO_BUFFER_MASK);

    return true;
}

/**
 * @brief Inspect the next byte without advancing tail (non-destructive read).
 *
 * Useful for protocol parsers that need to decide whether to consume a byte
 * based on its value before committing.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Output pointer; receives the peeked byte on success.
 * @return true  A byte was available; *data is valid.
 * @return false Buffer empty; *data is not modified.
 */
bool FIFO_Peek(FIFO_t *fifo, uint8_t *data)
{
    if (fifo->head == fifo->tail)
    {
        return false;
    }

    /* Copy without moving tail. */
    *data = fifo->buffer[fifo->tail];

    return true;
}

/**
 * @brief Check whether the FIFO is empty.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return true  head == tail → no data available.
 * @return false At least one byte is waiting.
 */
bool FIFO_IsEmpty(const FIFO_t *fifo)
{
    return (fifo->head == fifo->tail);
}

/**
 * @brief Check whether the FIFO is full.
 *
 * Full is defined as: the slot immediately before tail (wrapping) equals head.
 * One slot is always kept empty to distinguish full from empty without an extra
 * counter variable.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return true  Buffer is full; the next FIFO_Put() will fail.
 * @return false At least one byte of space remains.
 */
bool FIFO_IsFull(const FIFO_t *fifo)
{
    uint8_t next_head = (uint8_t)((fifo->head + 1u) & FIFO_BUFFER_MASK);
    return (next_head == fifo->tail);
}

/**
 * @brief Return the number of bytes currently stored.
 *
 * The calculation handles the wrap-around case (head < tail) by adding
 * FIFO_BUFFER_SIZE before subtracting, then masking to the valid range.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return Byte count in the range [0, FIFO_BUFFER_SIZE - 1].
 */
uint8_t FIFO_Count(const FIFO_t *fifo)
{
    return (uint8_t)((fifo->head - fifo->tail) & FIFO_BUFFER_MASK);
}
