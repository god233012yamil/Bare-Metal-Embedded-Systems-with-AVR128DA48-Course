/*
 * ring_buffer.h
 *
 * Generic Ring Buffer (Circular Buffer) Implementation
 * Thread-safe for single producer/single consumer scenarios
 *
 * A ring buffer is a fixed-size FIFO data structure that wraps around
 * when it reaches the end, allowing efficient use of memory without
 * data movement.
 *
 * Created for AVR128DA48 USART Interrupt-Driven Example
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Ring buffer structure
 * 
 * This structure maintains the state of a circular buffer including:
 * - The data array
 * - Read index (head) - where data is removed
 * - Write index (tail) - where data is added
 * - Buffer size
 * 
 * The buffer is full when (tail + 1) % size == head
 * The buffer is empty when tail == head
 */
typedef struct {
    volatile uint8_t *buffer;      ///< Pointer to the data buffer array
    volatile uint16_t head;        ///< Read index (removal point)
    volatile uint16_t tail;        ///< Write index (insertion point)
    uint16_t size;                 ///< Maximum buffer size
} ring_buffer_t;

/**
 * @brief Initialize a ring buffer
 * 
 * This function sets up a ring buffer with the provided storage array.
 * Both head and tail indices are set to 0 (empty state).
 * 
 * @param rb Pointer to the ring buffer structure
 * @param buffer Pointer to the storage array
 * @param size Size of the storage array
 * @return None
 */
static inline void ring_buffer_init(ring_buffer_t *rb, volatile uint8_t *buffer, uint16_t size)
{
    rb->buffer = buffer;
    rb->head = 0;
    rb->tail = 0;
    rb->size = size;
}

/**
 * @brief Check if the ring buffer is empty
 * 
 * The buffer is empty when the head (read index) equals the tail (write index).
 * 
 * @param rb Pointer to the ring buffer structure
 * @return true if buffer is empty, false otherwise
 */
static inline bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    return (rb->head == rb->tail);
}

/**
 * @brief Check if the ring buffer is full
 * 
 * The buffer is full when advancing the tail by one position would equal the head.
 * We sacrifice one buffer position to distinguish between full and empty states.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return true if buffer is full, false otherwise
 */
static inline bool ring_buffer_is_full(ring_buffer_t *rb)
{
    return ((rb->tail + 1) % rb->size) == rb->head;
}

/**
 * @brief Get the number of bytes available in the ring buffer
 * 
 * Calculates how many bytes are currently stored in the buffer.
 * Handles wrap-around using modulo arithmetic.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return Number of bytes available to read
 */
static inline uint16_t ring_buffer_available(ring_buffer_t *rb)
{
    return (rb->tail - rb->head + rb->size) % rb->size;
}

/**
 * @brief Get the free space available in the ring buffer
 * 
 * Calculates how many more bytes can be written to the buffer.
 * Accounts for the one position we keep empty to distinguish full from empty.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return Number of bytes that can be written
 */
static inline uint16_t ring_buffer_free_space(ring_buffer_t *rb)
{
    return (rb->size - 1 - ring_buffer_available(rb));
}

/**
 * @brief Write a byte to the ring buffer
 * 
 * Adds a byte to the buffer at the tail position and advances the tail.
 * This function does NOT check if the buffer is full - caller must verify.
 * 
 * Thread Safety: Safe when called from one context (e.g., only interrupts)
 * 
 * @param rb Pointer to the ring buffer structure
 * @param data Byte to write to the buffer
 * @return true if successful, false if buffer was full
 */
static inline bool ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    // Check if buffer is full
    if (ring_buffer_is_full(rb)) {
        return false;  // Buffer full, write failed
    }
    
    // Write data and advance tail
    rb->buffer[rb->tail] = data;
    rb->tail = (rb->tail + 1) % rb->size;
    
    return true;
}

/**
 * @brief Read a byte from the ring buffer
 * 
 * Removes and returns a byte from the head position and advances the head.
 * This function does NOT check if the buffer is empty - caller must verify.
 * 
 * Thread Safety: Safe when called from one context (e.g., only main loop)
 * 
 * @param rb Pointer to the ring buffer structure
 * @param data Pointer to store the read byte
 * @return true if successful, false if buffer was empty
 */
static inline bool ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    // Check if buffer is empty
    if (ring_buffer_is_empty(rb)) {
        return false;  // Buffer empty, read failed
    }
    
    // Read data and advance head
    *data = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % rb->size;
    
    return true;
}

/**
 * @brief Peek at a byte in the ring buffer without removing it
 * 
 * Returns a byte from the head position without advancing the head.
 * Useful for examining data before deciding to consume it.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param data Pointer to store the peeked byte
 * @return true if successful, false if buffer was empty
 */
static inline bool ring_buffer_peek(ring_buffer_t *rb, uint8_t *data)
{
    // Check if buffer is empty
    if (ring_buffer_is_empty(rb)) {
        return false;  // Buffer empty, peek failed
    }
    
    // Read data without advancing head
    *data = rb->buffer[rb->head];
    
    return true;
}

/**
 * @brief Clear/flush the ring buffer
 * 
 * Resets the buffer to empty state by setting head equal to tail.
 * Does not actually erase data, just makes it inaccessible.
 * 
 * @param rb Pointer to the ring buffer structure
 * @return None
 */
static inline void ring_buffer_flush(ring_buffer_t *rb)
{
    rb->head = rb->tail;
}

#endif /* RING_BUFFER_H_ */
