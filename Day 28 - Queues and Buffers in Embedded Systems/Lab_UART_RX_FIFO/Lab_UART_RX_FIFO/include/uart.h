/**
 * @file uart.h
 * @brief UART driver interface for the AVR128DA48, using USART1 (PC0/PC1).
 *
 * Configures USART1 for interrupt-driven receive and polled transmit. Received
 * bytes are stored automatically in a FIFO buffer by the RXC interrupt service
 * routine; the application retrieves them via UART_GetByte() / UART_ByteAvailable()
 * from the main loop.
 *
 * Default pin mapping (USART1, MUX default):
 *   - PC0  →  TXD
 *   - PC1  →  RXD
 *
 * These match the USART1 CDC connection on the AVR128DA48 Curiosity Nano board
 * when the on-board debugger's virtual COM port is used.
 *
 * Baud rate and CPU frequency are configured via the macros below. Adjust
 * F_CPU to match your project's clock configuration (default: 4 MHz internal
 * oscillator as shipped).
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>
#include "fifo.h"

/* -------------------------------------------------------------------------
 * Configuration macros – adjust to match your hardware / clock settings
 * ---------------------------------------------------------------------- */

/** CPU frequency in Hz. Must match the actual device clock. */
#ifndef F_CPU
#define F_CPU  4000000UL
#endif

/** Desired USART baud rate in bps. */
#define UART_BAUD_RATE  9600UL

/**
 * @brief USART baud register value calculated from F_CPU and UART_BAUD_RATE.
 *
 * Formula from the AVR-Dx datasheet (normal-speed asynchronous mode):
 *   BAUD = (64 * F_CPU) / (16 * baud_rate)
 *        = (4 * F_CPU) / baud_rate
 */
#define UART_BAUD_VALUE  ((uint16_t)((4UL * F_CPU) / UART_BAUD_RATE))

/* -------------------------------------------------------------------------
 * Global FIFO instance (defined in uart.c, declared here for ISR access)
 * ---------------------------------------------------------------------- */

/** Global receive FIFO populated by the USART1 RXC ISR. */
extern FIFO_t uart_rx_fifo;

/* -------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise USART1 for 8-N-1 interrupt-driven RX and polled TX.
 *
 * Must be called once before any other UART function. Enables global
 * interrupts (sei()) as a side-effect.
 *
 * @param none
 */
void UART_Init(void);

/**
 * @brief Transmit a single byte (blocking / polled).
 *
 * Waits until the USART Data Register Empty (DREIF) flag is set, then writes
 * the byte. Returns only after the byte has been accepted by the transmit
 * shift register.
 *
 * @param data  Byte to transmit.
 */
void UART_SendByte(uint8_t data);

/**
 * @brief Transmit a null-terminated string (blocking / polled).
 *
 * Calls UART_SendByte() for each character in the string until the terminating
 * null byte is reached. The null byte itself is not transmitted.
 *
 * @param str  Pointer to a null-terminated character string.
 */
void UART_SendString(const char *str);

/**
 * @brief Retrieve one byte from the receive FIFO.
 *
 * Non-blocking: returns immediately with false if no data is available.
 *
 * @param data  Output pointer; receives the byte when one is available.
 * @return true  A byte was retrieved and stored in *data.
 * @return false Receive FIFO is empty; *data is not modified.
 */
bool UART_GetByte(uint8_t *data);

/**
 * @brief Check whether at least one byte is waiting in the receive FIFO.
 *
 * @return true  One or more bytes are available.
 * @return false Receive FIFO is empty.
 */
bool UART_ByteAvailable(void);

#endif /* UART_H_ */
