/**
 * @file fifo.h
 * @brief FIFO (First-In, First-Out) circular buffer interface for UART receive buffering.
 *
 * Provides a fixed-size, interrupt-safe circular buffer suitable for use as a
 * UART receive FIFO on AVR128DA48 and similar AVR-Dx devices. The buffer stores
 * raw bytes (uint8_t) and is designed to be written from an ISR and read from
 * the main application loop.
 *
 * Usage:
 *  1. Declare a FIFO_t variable (or use the global uart_rx_fifo).
 *  2. Call FIFO_Init() before use.
 *  3. Call FIFO_Put() from the UART RX ISR.
 *  4. Call FIFO_Get() / FIFO_Peek() from the main loop.
 *
 * @note FIFO_BUFFER_SIZE must be a power of two for the masking optimisation
 *       to work correctly.
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------------- */

/** Number of bytes the FIFO can hold. MUST be a power of two (e.g. 16, 32, 64). */
#define FIFO_BUFFER_SIZE  64u

/** Bitmask derived from buffer size – used for fast modulo-free wrapping. */
#define FIFO_BUFFER_MASK  (FIFO_BUFFER_SIZE - 1u)

/* -------------------------------------------------------------------------
 * Data types
 * ---------------------------------------------------------------------- */

/**
 * @brief FIFO control structure.
 *
 * All members are managed exclusively through the FIFO_* API; do not access
 * them directly.
 */
typedef struct
{
    volatile uint8_t  head;                    /**< Write index (updated by producer / ISR). */
    volatile uint8_t  tail;                    /**< Read  index (updated by consumer / main). */
    uint8_t           buffer[FIFO_BUFFER_SIZE]; /**< Backing storage array. */
} FIFO_t;

/* -------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise (or reset) a FIFO instance.
 * @param fifo  Pointer to the FIFO_t to initialise. Must not be NULL.
 */
void    FIFO_Init   (FIFO_t *fifo);

/**
 * @brief Write one byte into the FIFO.
 *
 * Intended to be called from an ISR. Returns false and discards the byte when
 * the buffer is full (overflow protection).
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Byte to store.
 * @return true  Byte stored successfully.
 * @return false Buffer was full; byte discarded.
 */
bool    FIFO_Put    (FIFO_t *fifo, uint8_t data);

/**
 * @brief Read and remove one byte from the FIFO.
 *
 * Intended to be called from the main application loop.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Output pointer where the retrieved byte is stored.
 * @return true  A byte was available and written to *data.
 * @return false Buffer was empty; *data is unchanged.
 */
bool    FIFO_Get    (FIFO_t *fifo, uint8_t *data);

/**
 * @brief Inspect the next byte without removing it from the FIFO.
 *
 * @param fifo  Pointer to an initialised FIFO_t.
 * @param data  Output pointer where the peeked byte is stored.
 * @return true  A byte was available and written to *data.
 * @return false Buffer was empty; *data is unchanged.
 */
bool    FIFO_Peek   (FIFO_t *fifo, uint8_t *data);

/**
 * @brief Query whether the FIFO contains at least one byte.
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return true  One or more bytes are available.
 * @return false Buffer is empty.
 */
bool    FIFO_IsEmpty(const FIFO_t *fifo);

/**
 * @brief Query whether the FIFO is completely full.
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return true  Buffer is full; the next FIFO_Put() call will fail.
 * @return false Buffer has room for at least one more byte.
 */
bool    FIFO_IsFull (const FIFO_t *fifo);

/**
 * @brief Return the number of bytes currently stored in the FIFO.
 * @param fifo  Pointer to an initialised FIFO_t.
 * @return Number of bytes available to read (0 … FIFO_BUFFER_SIZE).
 */
uint8_t FIFO_Count  (const FIFO_t *fifo);

#endif /* FIFO_H_ */
