/**
 * @file    fifo.h
 * @brief   Generic lock-free single-producer / single-consumer ring buffer.
 *
 * This module provides a fixed-size, power-of-two FIFO (ring buffer) that is
 * safe for use in an ISR-producer / main-consumer pattern without disabling
 * interrupts, provided that only one writer and one reader exist at a time.
 * The implementation uses volatile head/tail indices and no dynamic allocation.
 *
 * Usage:
 * @code
 *   Fifo_t rxFifo;
 *   uint8_t rxBuf[FIFO_RX_SIZE];
 *   FIFO_Init(&rxFifo, rxBuf, FIFO_RX_SIZE);
 *
 *   // In ISR:
 *   FIFO_PutByte(&rxFifo, data);
 *
 *   // In main loop:
 *   uint8_t byte;
 *   if (FIFO_GetByte(&rxFifo, &byte)) { ... }
 * @endcode
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Data types
 * ========================================================================= */

/**
 * @brief Ring-buffer control structure.
 *
 * Members are volatile because the head is written by an ISR and read by the
 * main loop, and the tail vice versa.
 */
typedef struct
{
    uint8_t         *buf;       /**< Pointer to the backing storage array.    */
    volatile uint8_t head;      /**< Write index (producer advances this).    */
    volatile uint8_t tail;      /**< Read  index (consumer advances this).    */
    uint8_t          mask;      /**< Capacity minus 1 (power-of-two mask).    */
} Fifo_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes a FIFO control structure.
 *
 * @param[out] fifo     Pointer to the FIFO to initialize.
 * @param[in]  buf      Backing storage array (must remain valid for the
 *                      lifetime of the FIFO).
 * @param[in]  size     Number of bytes in @p buf.  Must be a power of 2
 *                      and ≤ 256.
 */
void FIFO_Init(Fifo_t *fifo, uint8_t *buf, uint8_t size);

/**
 * @brief Attempts to write one byte into the FIFO.
 *
 * Safe to call from an ISR.  Returns false without blocking if the buffer
 * is full.
 *
 * @param[in,out] fifo  The target FIFO.
 * @param[in]     byte  The byte to enqueue.
 * @return true   Byte successfully stored.
 * @return false  Buffer was full; byte discarded.
 */
bool FIFO_PutByte(Fifo_t *fifo, uint8_t byte);

/**
 * @brief Attempts to read one byte from the FIFO.
 *
 * Must be called from the consumer context only (main loop for an ISR-filled
 * buffer).  Returns false without blocking if the buffer is empty.
 *
 * @param[in,out] fifo  The source FIFO.
 * @param[out]    byte  Receives the dequeued byte.  Unchanged on failure.
 * @return true   Byte successfully retrieved.
 * @return false  Buffer was empty.
 */
bool FIFO_GetByte(Fifo_t *fifo, uint8_t *byte);

/**
 * @brief Returns the number of bytes currently stored in the FIFO.
 *
 * @param[in] fifo  The FIFO to query.
 * @return Number of bytes available to read (0 if empty).
 */
uint8_t FIFO_Count(const Fifo_t *fifo);

/**
 * @brief Reports whether the FIFO is empty.
 *
 * @param[in] fifo  The FIFO to query.
 * @return true  if no bytes are available.
 */
bool FIFO_IsEmpty(const Fifo_t *fifo);

/**
 * @brief Reports whether the FIFO is full.
 *
 * @param[in] fifo  The FIFO to query.
 * @return true  if no more bytes can be written.
 */
bool FIFO_IsFull(const Fifo_t *fifo);

#endif /* FIFO_H_ */
