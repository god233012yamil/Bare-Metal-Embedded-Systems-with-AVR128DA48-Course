/*
 * usart0_interrupt.h
 *
 * Interrupt-Driven USART0 Driver with Ring Buffers for AVR128DA48
 * 
 * This driver provides non-blocking USART communication using:
 * - Receive Complete Interrupt (RXC) for incoming data
 * - Data Register Empty Interrupt (DRE) for outgoing data
 * - Ring buffers for both TX and RX
 *
 * Created with Microchip Studio 7
 */

#ifndef USART0_INTERRUPT_H_
#define USART0_INTERRUPT_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

// Buffer size definitions
// Must be powers of 2 for optimal performance (though not required with modulo)
// Larger buffers = more memory usage but less chance of overflow
#define USART0_RX_BUFFER_SIZE 128   ///< Receive buffer size in bytes
#define USART0_TX_BUFFER_SIZE 128   ///< Transmit buffer size in bytes

// Function prototypes

/**
 * @brief Initialize USART0 with interrupts enabled
 * 
 * Configures USART0 for interrupt-driven operation:
 * - Sets baud rate to 9600 bps (configurable)
 * - Enables RX complete interrupt
 * - Enables TX and RX hardware
 * - Initializes TX and RX ring buffers
 * - Configures GPIO pins (PA0=TxD, PA1=RxD)
 * 
 * Note: Global interrupts must be enabled separately with sei()
 * 
 * @param None
 * @return None
 */
void USART0_init(void);

/**
 * @brief Send a single byte via USART0 (non-blocking)
 * 
 * Adds a byte to the transmit buffer. If the buffer is full, the function
 * returns false immediately without blocking.
 * 
 * The Data Register Empty interrupt will be automatically enabled to
 * transmit the buffered data in the background.
 * 
 * @param data Byte to transmit
 * @return true if byte was buffered, false if TX buffer is full
 */
bool USART0_write(uint8_t data);

/**
 * @brief Send a null-terminated string via USART0 (non-blocking)
 * 
 * Attempts to buffer an entire string for transmission. If the TX buffer
 * doesn't have enough space for the complete string, no data is sent and
 * the function returns false.
 * 
 * @param str Pointer to null-terminated string
 * @return true if entire string was buffered, false if insufficient space
 */
bool USART0_writeString(const char *str);

/**
 * @brief Send a string with automatic retry until complete
 * 
 * This function blocks until the entire string has been buffered for
 * transmission. It will wait if the TX buffer is full.
 * 
 * Use this when you need guaranteed transmission and don't mind blocking.
 * 
 * @param str Pointer to null-terminated string
 * @return None
 */
void USART0_writeString_blocking(const char *str);

/**
 * @brief Read a byte from the receive buffer (non-blocking)
 * 
 * Retrieves one byte from the receive buffer if available.
 * If no data is available, returns false immediately.
 * 
 * Data is received in the background by the RXC interrupt and stored
 * in the ring buffer.
 * 
 * @param data Pointer to store the received byte
 * @return true if byte was read, false if RX buffer is empty
 */
bool USART0_read(uint8_t *data);

/**
 * @brief Check if data is available in the receive buffer
 * 
 * Returns the number of bytes waiting in the RX buffer.
 * Useful for checking before calling USART0_read() or for reading
 * multiple bytes in a loop.
 * 
 * @param None
 * @return Number of bytes available (0 if empty)
 */
uint16_t USART0_available(void);

/**
 * @brief Get free space in transmit buffer
 * 
 * Returns how many bytes can be written to the TX buffer without
 * blocking or failing.
 * 
 * @param None
 * @return Number of free bytes in TX buffer
 */
uint16_t USART0_txFreeSpace(void);

/**
 * @brief Check if all transmitted data has been sent
 * 
 * Returns true when both the TX buffer is empty AND the hardware
 * transmit shift register has finished sending the last byte.
 * 
 * Useful when you need to ensure all data has been physically transmitted
 * before sleeping, changing baud rate, or disabling USART.
 * 
 * @param None
 * @return true if transmission is complete, false if data is still being sent
 */
bool USART0_txComplete(void);

/**
 * @brief Flush (clear) the receive buffer
 * 
 * Discards all data in the RX buffer. Useful for clearing old/invalid
 * data before starting a new communication sequence.
 * 
 * @param None
 * @return None
 */
void USART0_flushRx(void);

/**
 * @brief Flush (clear) the transmit buffer
 * 
 * Discards all pending data in the TX buffer. Note that data already
 * loaded into the hardware transmit register will still be sent.
 * 
 * @param None
 * @return None
 */
void USART0_flushTx(void);

/**
 * @brief Print a formatted string (simple printf-like function)
 * 
 * Supports basic format specifiers:
 * %d - signed decimal integer
 * %u - unsigned decimal integer
 * %x - hexadecimal (lowercase)
 * %X - hexadecimal (uppercase)
 * %c - character
 * %s - string
 * %% - literal '%'
 * 
 * @param format Format string with specifiers
 * @param ... Variable arguments matching format specifiers
 * @return None
 */
void USART0_printf(const char *format, ...);

#endif /* USART0_INTERRUPT_H_ */
