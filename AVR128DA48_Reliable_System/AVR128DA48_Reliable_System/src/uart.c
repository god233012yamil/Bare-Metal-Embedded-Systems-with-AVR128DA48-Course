#include <avr/io.h>
#include <stdbool.h>
#include "config.h"
#include "system_time.h"
#include "uart.h"

/**
 * @brief Initializes USART0 for 8N1 asynchronous serial diagnostics.
 *
 * Args:
 *     baud_rate: Requested UART baud rate.
 *
 * Returns:
 *     None.
 */
void uart_init(uint32_t baud_rate)
{
    PORTA.DIRSET = PIN0_bm;
    PORTA.DIRCLR = PIN1_bm;

    USART0.BAUD = (uint16_t)((64UL * F_CPU) / (16UL * baud_rate));
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

/**
 * @brief Writes one byte to USART0 with timeout protection.
 *
 * Args:
 *     data: Byte to transmit.
 *     timeout_ms: Maximum time to wait for the TX register.
 *
 * Returns:
 *     STATUS_OK on success, otherwise STATUS_TIMEOUT.
 */
status_t uart_write_byte(uint8_t data, uint32_t timeout_ms)
{
    uint32_t start_ms = system_time_get_ms();

    while ((USART0.STATUS & USART_DREIF_bm) == 0U)
    {
        if (system_time_elapsed(start_ms, timeout_ms))
        {
            return STATUS_TIMEOUT;
        }
    }

    USART0.TXDATAL = data;

    return STATUS_OK;
}

/**
 * @brief Writes a null-terminated string to USART0.
 *
 * Args:
 *     text: String to transmit.
 *
 * Returns:
 *     STATUS_OK on success, otherwise an error code.
 */
status_t uart_write_string(const char *text)
{
    if (text == 0)
    {
        return STATUS_INVALID_ARG;
    }

    while (*text != '\0')
    {
        status_t status = uart_write_byte((uint8_t)*text, UART_TX_TIMEOUT_MS);

        if (status != STATUS_OK)
        {
            return status;
        }

        text++;
    }

    return STATUS_OK;
}

/**
 * @brief Writes an unsigned 32-bit integer in decimal format.
 *
 * Args:
 *     value: Value to transmit.
 *
 * Returns:
 *     STATUS_OK on success, otherwise an error code.
 */
status_t uart_write_u32(uint32_t value)
{
    char buffer[11];
    uint8_t index = 0;

    if (value == 0U)
    {
        return uart_write_byte('0', UART_TX_TIMEOUT_MS);
    }

    while ((value > 0U) && (index < sizeof(buffer)))
    {
        buffer[index] = (char)('0' + (value % 10U));
        value /= 10U;
        index++;
    }

    while (index > 0U)
    {
        index--;

        if (uart_write_byte((uint8_t)buffer[index], UART_TX_TIMEOUT_MS) != STATUS_OK)
        {
            return STATUS_TIMEOUT;
        }
    }

    return STATUS_OK;
}

/**
 * @brief Writes an 8-bit value in hexadecimal format.
 *
 * Args:
 *     value: Value to transmit.
 *
 * Returns:
 *     STATUS_OK on success, otherwise an error code.
 */
status_t uart_write_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    if (uart_write_byte((uint8_t)hex[(value >> 4) & 0x0FU], UART_TX_TIMEOUT_MS) != STATUS_OK)
    {
        return STATUS_TIMEOUT;
    }

    return uart_write_byte((uint8_t)hex[value & 0x0FU], UART_TX_TIMEOUT_MS);
}
