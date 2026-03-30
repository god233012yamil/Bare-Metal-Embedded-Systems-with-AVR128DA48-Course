#include "uart.h"
#include <avr/io.h>
#include <stdint.h>

void uart_init(void)
{
    /* PC0 = TX output, PC1 = RX input */
    PORTC.DIRSET = PIN0_bm;
    PORTC.DIRCLR = PIN1_bm;

    /* Route USART0 to alternate pins PC0/PC1 - default PORTMUX selection */
    /* PORTMUX.USARTROUTEA default = USART0 on PC0/PC1 (no change needed) */

    USART0.BAUD    = UART_BAUD_REG;
    USART0.CTRLB   = USART_TXEN_bm | USART_RXEN_bm;
    USART0.CTRLC   = USART_CMODE_ASYNCHRONOUS_gc
                   | USART_PMODE_DISABLED_gc
                   | USART_SBMODE_1BIT_gc
                   | USART_CHSIZE_8BIT_gc;
}

void uart_send_byte(uint8_t byte)
{
    while (!(USART0.STATUS & USART_DREIF_bm))
    {
        ;
    }
    USART0.TXDATAL = byte;
}

void uart_print_string(const char *str)
{
    while (*str)
    {
        uart_send_byte((uint8_t)*str++);
    }
}

void uart_print_uint16(uint16_t val)
{
    char buf[6];
    uint8_t i = 0;

    if (val == 0)
    {
        uart_send_byte('0');
        return;
    }

    while (val > 0)
    {
        buf[i++] = (char)('0' + (val % 10));
        val /= 10;
    }

    while (i > 0)
    {
        uart_send_byte((uint8_t)buf[--i]);
    }
}

void uart_print_uint32(uint32_t val)
{
    char buf[11];
    uint8_t i = 0;

    if (val == 0)
    {
        uart_send_byte('0');
        return;
    }

    while (val > 0)
    {
        buf[i++] = (char)('0' + (val % 10));
        val /= 10;
    }

    while (i > 0)
    {
        uart_send_byte((uint8_t)buf[--i]);
    }
}
