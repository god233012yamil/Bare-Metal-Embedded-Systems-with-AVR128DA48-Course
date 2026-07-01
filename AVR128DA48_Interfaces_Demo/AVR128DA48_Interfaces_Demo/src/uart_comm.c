#include "uart_comm.h"

#include <avr/io.h>
#include <stddef.h>

#ifndef F_CPU
#define F_CPU 4000000UL
#endif

/**
 * Calculates the AVR Dx asynchronous USART baud register value.
 *
 * Args:
 *   baudrate: Requested baud rate in bits per second.
 *
 * Returns:
 *   Baud register value for normal asynchronous mode.
 */
static uint16_t uart_baud_value(uint32_t baudrate)
{
    return (uint16_t)((64UL * F_CPU) / (16UL * baudrate));
}

/**
 * Writes bytes using a USART-backed communication interface.
 *
 * Args:
 *   context: Pointer to a uart_comm_context_t instance.
 *   data: Pointer to the bytes to transmit.
 *   length: Number of bytes to transmit.
 *
 * Returns:
 *   Number of bytes transmitted, or -1 if the arguments are invalid.
 */
static int uart_write_impl(void *context, const uint8_t *data, size_t length)
{
    uart_comm_context_t *uart = (uart_comm_context_t *)context;

    if ((uart == NULL) || (data == NULL)) {
        return -1;
    }

    for (size_t i = 0U; i < length; i++) {
        while ((uart->usart->STATUS & USART_DREIF_bm) == 0U) {
            /* Wait until the transmit data register is empty. */
        }

        uart->usart->TXDATAL = data[i];
    }

    return (int)length;
}

/**
 * Reads bytes using a USART-backed communication interface.
 *
 * Args:
 *   context: Pointer to a uart_comm_context_t instance.
 *   data: Destination buffer.
 *   length: Maximum number of bytes to read.
 *
 * Returns:
 *   Number of bytes read. This non-blocking demo returns available bytes only.
 */
static int uart_read_impl(void *context, uint8_t *data, size_t length)
{
    uart_comm_context_t *uart = (uart_comm_context_t *)context;
    size_t count = 0U;

    if ((uart == NULL) || (data == NULL)) {
        return -1;
    }

    while ((count < length) && ((uart->usart->STATUS & USART_RXCIF_bm) != 0U)) {
        data[count] = uart->usart->RXDATAL;
        count++;
    }

    return (int)count;
}

/**
 * Creates a USART implementation of the generic communication interface.
 *
 * Args:
 *   interface: Generic communication interface to initialize.
 *   context: USART communication private state storage.
 *   usart: AVR Dx USART peripheral instance.
 *   baudrate: Requested baud rate in bits per second.
 */
void uart_comm_create(comm_interface_t *interface,
                      uart_comm_context_t *context,
                      USART_t *usart,
                      uint32_t baudrate)
{
    context->usart = usart;

    PORTC.DIRSET = PIN0_bm;
    PORTC.DIRCLR = PIN1_bm;

    usart->BAUD = uart_baud_value(baudrate);
    usart->CTRLC = USART_CHSIZE_8BIT_gc;
    usart->CTRLB = USART_TXEN_bm | USART_RXEN_bm;

    interface->context = context;
    interface->write = uart_write_impl;
    interface->read = uart_read_impl;
}
