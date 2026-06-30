#include "uart.h"

#include <avr/io.h>
#include <stddef.h>

#ifndef F_CPU
#define F_CPU 24000000UL
#endif

#define UART_BAUD_RATE 115200UL
#define UART_BAUD_VALUE ((uint16_t)((4UL * F_CPU) / UART_BAUD_RATE))

/**
 * Initializes USART0 for diagnostic serial output.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void uart_init(void)
{
    // Route USART0 to the default PA0 TX and PA1 RX pins.
    PORTA.OUTSET = PIN0_bm;
    PORTA.DIRSET = PIN0_bm;
    PORTA.DIRCLR = PIN1_bm;
    PORTA.PIN1CTRL = PORT_PULLUPEN_bm;

    USART0.BAUD = UART_BAUD_VALUE;
    USART0.CTRLC = USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc;
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

/**
 * Writes one character to USART0.
 *
 * Args:
 *     character: Character to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_char(char character)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0U)
    {
        // Wait until the transmit data register can accept another byte.
    }

    USART0.TXDATAL = (uint8_t)character;
}

/**
 * Writes a null-terminated string to USART0.
 *
 * Args:
 *     text: String to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_string(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        uart_write_char(*text);
        text++;
    }
}

/**
 * Writes an unsigned 32-bit integer to USART0 as decimal text.
 *
 * Args:
 *     value: Integer value to transmit.
 *
 * Returns:
 *     None.
 */
void uart_write_u32(uint32_t value)
{
    char digits[10];
    uint8_t count = 0U;

    if (value == 0U)
    {
        uart_write_char('0');
        return;
    }

    while ((value > 0U) && (count < (uint8_t)sizeof(digits)))
    {
        digits[count] = (char)('0' + (value % 10U));
        value /= 10U;
        count++;
    }

    while (count > 0U)
    {
        count--;
        uart_write_char(digits[count]);
    }
}
