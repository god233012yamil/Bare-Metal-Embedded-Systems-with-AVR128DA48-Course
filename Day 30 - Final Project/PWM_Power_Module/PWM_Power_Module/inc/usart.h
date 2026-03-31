/**
 * @file    usart.h
 * @brief   USART0 driver with interrupt-driven RX FIFO and polled TX.
 *
 * Provides a lightweight USART driver for the AVR128DA48 Curiosity Nano.
 * USART0 is mapped to its default pins (PA0 = TXD, PA1 = RXD), which are
 * connected to the on-board CDC-USB bridge, making it the natural debug /
 * telemetry channel.
 *
 * Architecture:
 *  - RX: interrupt-driven.  The RXCIF ISR deposits bytes into a FIFO
 *        (see fifo.h).  The application calls USART_GetByte() to drain it.
 *  - TX: polled (blocking per character).  Acceptable for telemetry frames
 *        sent infrequently; upgrade to TX FIFO + DRE ISR if needed.
 *
 * Hardware resources used:
 *  - USART0
 *  - PORTA PIN0 (TXD, output), PORTA PIN1 (RXD, input).
 *  - PORTMUX: USART0 default mapping (PA0/PA1).
 *
 * Layer contract:
 *  - Application must use USART_* functions only.
 *  - Direct register access to USART0 is forbidden outside this module.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes USART0 for 115200 8N1 operation.
 *
 * Configures PORTMUX, pin directions, baud rate, frame format, enables the
 * transmitter and receiver, and enables the RXCIF interrupt.  The internal
 * RX FIFO is initialized to empty.
 *
 * Global interrupts must be enabled by the caller.
 */
void USART_Init(void);

/**
 * @brief Transmits a single byte (polled).
 *
 * Waits until the USART Data Register Empty flag (DREIF) is set, then
 * writes the byte.  Blocking; keep telemetry frames short.
 *
 * @param[in] byte  The byte to transmit.
 */
void USART_SendByte(uint8_t byte);

/**
 * @brief Transmits a null-terminated string.
 *
 * Iterates over the string calling USART_SendByte() for each character.
 * The null terminator is not transmitted.
 *
 * @param[in] str  Pointer to the null-terminated C string.  Must not be NULL.
 */
void USART_SendString(const char *str);

/**
 * @brief Transmits a 16-bit unsigned integer as a decimal ASCII string.
 *
 * Converts @p value to a decimal string (no leading zeros) and sends it.
 * Maximum output width is 5 characters (65535).
 *
 * @param[in] value  The value to print.
 */
void USART_SendUInt16(uint16_t value);

/**
 * @brief Attempts to retrieve one byte from the RX FIFO.
 *
 * Non-blocking.  Returns false immediately if no data is available.
 *
 * @param[out] byte  Receives the dequeued byte on success.
 * @return true  if a byte was retrieved.
 * @return false if the RX FIFO is empty.
 */
bool USART_GetByte(uint8_t *byte);

/**
 * @brief Reports whether the RX FIFO contains unread data.
 *
 * @return true if at least one byte is waiting.
 */
bool USART_RxAvailable(void);

#endif /* USART_H_ */
