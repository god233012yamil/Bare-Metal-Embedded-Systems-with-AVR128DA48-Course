/**
 * @file    fifo.c
 * @brief   Generic lock-free single-producer / single-consumer ring buffer.
 *
 * See fifo.h for full API documentation and usage notes.
 *
 * Implementation notes:
 *  - head and tail are uint8_t, so they wrap naturally at 256.  Combined with
 *    the power-of-two mask the FIFO capacity is always a power of two ≤ 128
 *    (a capacity of 256 would need uint16_t indices to detect full vs empty).
 *  - The "one slot wasted" convention is used: the buffer is considered full
 *    when (head + 1) & mask == tail, which means maximum usable capacity is
 *    (size - 1) bytes.  For FIFO_RX_SIZE = 64 the effective capacity is 63.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "fifo.h"
#include <stddef.h>

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises a FIFO control structure.
 *
 * @param[out] fifo  Target FIFO descriptor.
 * @param[in]  buf   Backing storage (caller-allocated, must outlive the FIFO).
 * @param[in]  size  Capacity in bytes; must be a power of 2 and ≤ 256.
 */
void FIFO_Init(Fifo_t *fifo, uint8_t *buf, uint8_t size)
{
    if ((fifo == NULL) || (buf == NULL) || (size == 0U))
    {
        return;
    }

    fifo->buf  = buf;
    fifo->head = 0U;
    fifo->tail = 0U;
    /* mask = size - 1 works only for powers of 2; caller is responsible for
     * passing a valid size (asserted in DEBUG builds via config.h). */
    fifo->mask = (uint8_t)(size - 1U);
}

/**
 * @brief Enqueues one byte (ISR-safe, non-blocking).
 *
 * @param[in,out] fifo  Target FIFO.
 * @param[in]     byte  Byte to store.
 * @return true on success, false if the buffer is full.
 */
bool FIFO_PutByte(Fifo_t *fifo, uint8_t byte)
{
    uint8_t nextHead;

    if (fifo == NULL)
    {
        return false;
    }

    nextHead = (uint8_t)((fifo->head + 1U) & fifo->mask);

    /* Full condition: next write position equals the read position. */
    if (nextHead == fifo->tail)
    {
        return false;
    }

    fifo->buf[fifo->head] = byte;
    /* The write to head must be visible AFTER the write to buf[].
     * avr-gcc with -O1/-O2 respects volatile ordering here because head is
     * volatile.  On architectures with weaker memory models a barrier would
     * be needed. */
    fifo->head = nextHead;

    return true;
}

/**
 * @brief Dequeues one byte (call from consumer context only).
 *
 * @param[in,out] fifo  Source FIFO.
 * @param[out]    byte  Receives the dequeued byte.
 * @return true on success, false if the buffer is empty.
 */
bool FIFO_GetByte(Fifo_t *fifo, uint8_t *byte)
{
    if ((fifo == NULL) || (byte == NULL))
    {
        return false;
    }

    /* Empty condition: head equals tail. */
    if (fifo->head == fifo->tail)
    {
        return false;
    }

    *byte = fifo->buf[fifo->tail];
    fifo->tail = (uint8_t)((fifo->tail + 1U) & fifo->mask);

    return true;
}

/**
 * @brief Returns the number of bytes available to read.
 *
 * @param[in] fifo  Source FIFO.
 * @return Byte count (0 if empty).
 */
uint8_t FIFO_Count(const Fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return 0U;
    }
    /* Cast away volatile for arithmetic; the snapshot is immediately used. */
    return (uint8_t)((fifo->head - fifo->tail) & fifo->mask);
}

/**
 * @brief Reports whether the FIFO holds no data.
 *
 * @param[in] fifo  Source FIFO.
 * @return true if empty.
 */
bool FIFO_IsEmpty(const Fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return true;
    }
    return (fifo->head == fifo->tail);
}

/**
 * @brief Reports whether the FIFO can accept no more data.
 *
 * @param[in] fifo  Source FIFO.
 * @return true if full.
 */
bool FIFO_IsFull(const Fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return true;
    }
    return (((fifo->head + 1U) & fifo->mask) == fifo->tail);
}
