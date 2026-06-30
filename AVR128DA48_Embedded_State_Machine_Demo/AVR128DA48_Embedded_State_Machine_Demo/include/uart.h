#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * Initializes USART0 for 115200 baud, 8 data bits, no parity, and one stop bit.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void uart_init(void);

/**
 * Sends one character through USART0.
 *
 * Args:
 *     character: Character to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_char(char character);

/**
 * Sends a null-terminated string through USART0.
 *
 * Args:
 *     text: Pointer to the string to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_string(const char *text);

/**
 * Sends an unsigned 32-bit integer as decimal text through USART0.
 *
 * Args:
 *     value: Integer value to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_u32(uint32_t value);

#endif
