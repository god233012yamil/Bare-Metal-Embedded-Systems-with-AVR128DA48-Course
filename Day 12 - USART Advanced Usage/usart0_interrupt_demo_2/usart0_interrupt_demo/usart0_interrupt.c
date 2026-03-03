/*
 * usart0_interrupt.c
 *
 * Interrupt-Driven USART0 Driver Implementation
 * Uses ring buffers for efficient non-blocking communication
 *
 * Created for AVR128DA48 with Microchip Studio 7
 */

#define F_CPU 4000000UL  // 4 MHz default clock frequency

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "usart0_interrupt.h"
#include "ring_buffer.h"

// Baud rate calculation macro
// BAUD = 64 * f_CLK / (S * BAUD_RATE)
// For normal mode (S=16): BAUD = 64 * 4000000 / (16 * 9600) = 1667
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(64 * F_CPU / (16 * (float)BAUD_RATE)) + 0.5)

// Static buffer storage (actual memory for ring buffers)
static volatile uint8_t rx_buffer_storage[USART0_RX_BUFFER_SIZE];
static volatile uint8_t tx_buffer_storage[USART0_TX_BUFFER_SIZE];

// Ring buffer structures
static ring_buffer_t rx_buffer;
static ring_buffer_t tx_buffer;

/**
 * @brief Initialize USART0 with interrupts enabled
 * 
 * This function configures USART0 for interrupt-driven operation:
 * 1. Initializes RX and TX ring buffers
 * 2. Sets baud rate to 9600 bps
 * 3. Enables transmitter and receiver hardware
 * 4. Configures frame format (8N1)
 * 5. Enables RX Complete Interrupt (data received automatically)
 * 6. Configures GPIO pins (PA0 as output for TxD, PA1 as input for RxD)
 * 
 * Note: This function does NOT enable global interrupts. 
 * Call sei() after initialization to enable interrupts globally.
 * 
 * Pin Configuration:
 * - PA0: TxD (Transmit Data) - Output
 * - PA1: RxD (Receive Data) - Input
 * 
 * @param None
 * @return None
 */
void USART0_init(void)
{
    // Initialize ring buffers
    ring_buffer_init(&rx_buffer, rx_buffer_storage, USART0_RX_BUFFER_SIZE);
    ring_buffer_init(&tx_buffer, tx_buffer_storage, USART0_TX_BUFFER_SIZE);
    
    // Configure baud rate to 9600 bps
    USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
    
    // Enable transmitter and receiver
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    
    // Configure frame format: 8 data bits, no parity, 1 stop bit
    USART0.CTRLC = USART_CHSIZE_8BIT_gc    // 8-bit character size
                 | USART_PMODE_DISABLED_gc  // No parity
                 | USART_SBMODE_1BIT_gc;    // 1 stop bit
    
    // Enable Receive Complete Interrupt
    // This interrupt fires when a byte is received
    USART0.CTRLA = USART_RXCIE_bm;
    
    // Note: Data Register Empty Interrupt (DREIE) is enabled dynamically
    // when there's data to send, and disabled when TX buffer is empty
    
    // Configure pins
    PORTA.DIRSET = PIN0_bm;  // PA0 (TxD) as output
    PORTA.DIRCLR = PIN1_bm;  // PA1 (RxD) as input (redundant but explicit)
}

/**
 * @brief Send a single byte via USART0 (non-blocking)
 * 
 * Attempts to add the byte to the transmit ring buffer. If successful,
 * enables the Data Register Empty interrupt to begin transmission in
 * the background.
 * 
 * The actual transmission is handled by the USART0_DRE_vect ISR, which
 * will be called repeatedly until the TX buffer is empty.
 * 
 * Thread Safety: This function disables the DRE interrupt temporarily
 * to prevent race conditions when accessing the shared TX buffer.
 * 
 * @param data Byte to transmit
 * @return true if byte was added to buffer, false if buffer is full
 */
bool USART0_write(uint8_t data)
{
    bool result;
    
    // Disable DRE interrupt to prevent race condition
    USART0.CTRLA &= ~USART_DREIE_bm;
    
    // Try to add byte to TX buffer
    result = ring_buffer_put(&tx_buffer, data);
    
    // If byte was buffered, enable DRE interrupt to start transmission
    if (result) {
        USART0.CTRLA |= USART_DREIE_bm;
    }
    
    return result;
}

/**
 * @brief Send a null-terminated string via USART0 (non-blocking)
 * 
 * Attempts to buffer the entire string at once. If there's insufficient
 * space in the TX buffer for the complete string, no data is sent and
 * the function returns false.
 * 
 * This "all or nothing" approach prevents partial string transmission
 * when the buffer is nearly full.
 * 
 * @param str Pointer to null-terminated string
 * @return true if entire string was buffered, false if insufficient space
 */
bool USART0_writeString(const char *str)
{
    uint16_t len = strlen(str);
    
    // Check if there's enough space for the entire string
    if (ring_buffer_free_space(&tx_buffer) < len) {
        return false;  // Not enough space
    }
    
    // Buffer the string character by character
    while (*str) {
        if (!USART0_write(*str++)) {
            return false;  // Should not happen since we checked space
        }
    }
    
    return true;
}

/**
 * @brief Send a string with blocking until complete
 * 
 * This function will wait (block) until there's space in the TX buffer
 * for each character. Use this when you need guaranteed transmission
 * and CPU efficiency isn't critical.
 * 
 * The function will return once all characters are buffered, but actual
 * transmission may still be in progress (handled by interrupts).
 * 
 * @param str Pointer to null-terminated string
 * @return None
 */
void USART0_writeString_blocking(const char *str)
{
    while (*str) {
        // Keep trying until character is buffered
        while (!USART0_write(*str)) {
            ; // Wait for space in buffer
        }
        str++;
    }
}

/**
 * @brief Read a byte from the receive buffer (non-blocking)
 * 
 * Attempts to retrieve one byte from the RX ring buffer. If the buffer
 * is empty, returns false immediately without blocking.
 * 
 * Data is automatically placed in the RX buffer by the RXC interrupt
 * handler when bytes are received.
 * 
 * Thread Safety: This function disables the RXC interrupt temporarily
 * to prevent the ISR from modifying the buffer while we're reading.
 * 
 * @param data Pointer to store the received byte
 * @return true if byte was read, false if buffer is empty
 */
bool USART0_read(uint8_t *data)
{
    bool result;
    
    // Disable RXC interrupt to prevent race condition
    uint8_t ctrla_backup = USART0.CTRLA;
    USART0.CTRLA &= ~USART_RXCIE_bm;
    
    // Try to get byte from RX buffer
    result = ring_buffer_get(&rx_buffer, data);
    
    // Restore interrupt state
    USART0.CTRLA = ctrla_backup;
    
    return result;
}

/**
 * @brief Check how many bytes are available in the receive buffer
 * 
 * Returns the count of bytes waiting to be read. This is useful for:
 * - Checking if data is available before calling USART0_read()
 * - Reading multiple bytes in a loop
 * - Implementing protocols that need to know data length
 * 
 * @param None
 * @return Number of bytes available (0 if buffer is empty)
 */
uint16_t USART0_available(void)
{
    return ring_buffer_available(&rx_buffer);
}

/**
 * @brief Get free space in the transmit buffer
 * 
 * Returns how many bytes can be written to the TX buffer without
 * blocking or failing. Useful for checking capacity before writing
 * large amounts of data.
 * 
 * @param None
 * @return Number of free bytes in TX buffer
 */
uint16_t USART0_txFreeSpace(void)
{
    return ring_buffer_free_space(&tx_buffer);
}

/**
 * @brief Check if all data has been transmitted
 * 
 * Returns true when:
 * 1. The TX ring buffer is empty (no more data to send), AND
 * 2. The hardware transmit shift register has finished sending
 * 
 * The TXCIF (Transmit Complete Interrupt Flag) is set when the last
 * byte has been completely shifted out of the hardware.
 * 
 * @param None
 * @return true if transmission is complete, false otherwise
 */
bool USART0_txComplete(void)
{
    return (ring_buffer_is_empty(&tx_buffer) && (USART0.STATUS & USART_TXCIF_bm));
}

/**
 * @brief Flush (clear) the receive buffer
 * 
 * Discards all unread data in the RX buffer by resetting the head
 * to equal the tail. Useful for clearing stale data before starting
 * a new communication sequence.
 * 
 * @param None
 * @return None
 */
void USART0_flushRx(void)
{
    // Disable RXC interrupt while flushing
    uint8_t ctrla_backup = USART0.CTRLA;
    USART0.CTRLA &= ~USART_RXCIE_bm;
    
    ring_buffer_flush(&rx_buffer);
    
    // Restore interrupt state
    USART0.CTRLA = ctrla_backup;
}

/**
 * @brief Flush (clear) the transmit buffer
 * 
 * Discards all pending data in the TX buffer. Note that any byte
 * already loaded into the hardware transmit register will still
 * be sent.
 * 
 * @param None
 * @return None
 */
void USART0_flushTx(void)
{
    // Disable DRE interrupt while flushing
    USART0.CTRLA &= ~USART_DREIE_bm;
    
    ring_buffer_flush(&tx_buffer);
}

/**
 * @brief Simple printf-like function for USART0
 * 
 * Provides basic formatted output with the following specifiers:
 * - %d: signed decimal integer
 * - %u: unsigned decimal integer
 * - %x: hexadecimal lowercase
 * - %X: hexadecimal uppercase
 * - %c: character
 * - %s: string
 * - %%: literal '%'
 * 
 * This is a simplified implementation. For full printf functionality,
 * consider using vsnprintf() with a buffer.
 * 
 * Example: USART0_printf("Value: %d, Hex: 0x%X\r\n", 255, 255);
 * 
 * @param format Format string with specifiers
 * @param ... Variable arguments
 * @return None
 */
void USART0_printf(const char *format, ...)
{
    char buffer[128];  // Temporary buffer for formatted output
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Send the formatted string (blocking version for reliability)
    USART0_writeString_blocking(buffer);
}

/**
 * @brief USART0 Receive Complete Interrupt Service Routine
 * 
 * This ISR is called automatically when a byte is received by USART0.
 * The RXCIF flag is automatically cleared when RXDATAL is read.
 * 
 * ISR Workflow:
 * 1. Read the received byte from RXDATAL register
 * 2. Attempt to add it to the RX ring buffer
 * 3. If buffer is full, the byte is discarded (overflow condition)
 * 
 * Note: If the RX buffer overflows frequently, consider:
 * - Increasing USART0_RX_BUFFER_SIZE
 * - Processing received data faster in main loop
 * - Implementing flow control (RTS/CTS)
 * 
 * @param None
 * @return None
 */
ISR(USART0_RXC_vect)
{
    uint8_t data;
    
    // Read received data (this also clears the RXCIF flag)
    data = USART0.RXDATAL;
    
    // Try to store in ring buffer
    // If buffer is full, data is lost (silent overflow)
    ring_buffer_put(&rx_buffer, data);
    
    // Optional: Implement overflow error handling here
    // Could set an error flag if ring_buffer_put() returns false
}

/**
 * @brief USART0 Data Register Empty Interrupt Service Routine
 * 
 * This ISR is called when the USART0 data register is empty and ready
 * to accept new data for transmission.
 * 
 * ISR Workflow:
 * 1. Check if there's data in the TX ring buffer
 * 2. If yes: Load next byte into TXDATAL register
 * 3. If no: Disable this interrupt (no more data to send)
 * 
 * The interrupt is re-enabled automatically when USART0_write() is called
 * with new data.
 * 
 * This on-demand interrupt enable/disable strategy prevents the ISR from
 * being called unnecessarily when there's nothing to transmit.
 * 
 * @param None
 * @return None
 */
ISR(USART0_DRE_vect)
{
    uint8_t data;
    
    // Try to get next byte from TX buffer
    if (ring_buffer_get(&tx_buffer, &data)) {
        // Send the byte
        USART0.TXDATAL = data;
    } else {
        // No more data to send, disable this interrupt
        USART0.CTRLA &= ~USART_DREIE_bm;
    }
}
