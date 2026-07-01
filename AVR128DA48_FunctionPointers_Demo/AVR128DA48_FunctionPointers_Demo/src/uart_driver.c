#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart_driver.h"
#include "global.h"

static uart_rx_callback_t rx_callback = 0;
static void *rx_context = 0;

/**
 * Initializes USART0 for asynchronous 8N1 operation.
 *
 * Args:
 *     baud_rate: Requested baud rate in bits per second.
 */
void uart0_init(uint32_t baud_rate)
{
    uint16_t baud_setting;

    PORTA.DIRSET = PIN0_bm;
    PORTA.DIRCLR = PIN1_bm;

    baud_setting = (uint16_t)((64UL * F_CPU) / (16UL * baud_rate));

    USART0.BAUD = baud_setting;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    USART0.CTRLA = USART_RXCIE_bm;
}

/**
 * Registers a receive callback for USART0.
 *
 * Args:
 *     callback: Function called from the USART RX interrupt.
 *     context: User context passed to the callback.
 */
void uart0_register_rx_callback(uart_rx_callback_t callback, void *context)
{
    cli();
    rx_callback = callback;
    rx_context = context;
    sei();
}

/**
 * Writes one byte through USART0.
 *
 * Args:
 *     data: Byte to transmit.
 */
void uart0_write_byte(uint8_t data)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0U)
    {
    }

    USART0.TXDATAL = data;
}

/**
 * Writes a null-terminated string through USART0.
 *
 * Args:
 *     text: Null-terminated string to transmit.
 */
void uart0_write_string(const char *text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        uart0_write_byte((uint8_t)*text);
        text++;
    }
}

/**
 * Handles USART0 receive-complete interrupts and forwards bytes through a callback.
 */
ISR(USART0_RXC_vect)
{
    uint8_t data = USART0.RXDATAL;

    if (rx_callback != 0)
    {
        rx_callback(data, rx_context);
    }
}
