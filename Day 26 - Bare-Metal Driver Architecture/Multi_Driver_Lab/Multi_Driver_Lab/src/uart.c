/*
 * uart.c  --  USART driver implementation, AVR128DA48
 *
 * Peripherals : USART0 (0x0800) – USART4 (0x0880)
 * All register symbols from ioavr128da48.h via <avr/io.h>.
 *
 * USART struct fields used (USART_t):
 *   TXDATAL   -- transmit data low byte
 *   STATUS    -- USART_DREIF_bm (data register empty)
 *   CTRLB     -- USART_TXEN_bm
 *   CTRLC     -- USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc
 *                USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc
 *   BAUD      -- 16-bit baud register (_WORDREGISTER)
 *
 * BAUD formula (async normal, S=16):
 *   BAUD_REG = (64 * F_CPU) / (16 * baud) = (4 * F_CPU) / baud
 */

#include "uart.h"
#include <avr/io.h>

/* -----------------------------------------------------------------------
 * Internal helper — return pointer to hardware USART instance
 * ----------------------------------------------------------------------- */
static USART_t *prv_inst(uart_instance_t inst)
{
    switch (inst) {
        case UART_INSTANCE_0: return &USART0;
        case UART_INSTANCE_1: return &USART1;
        case UART_INSTANCE_2: return &USART2;
        case UART_INSTANCE_3: return &USART3;
        case UART_INSTANCE_4: return &USART4;
        default:              return &USART0;
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void UART_Init(const uart_config_t *cfg)
{
    USART_t *u = prv_inst(cfg->instance);

    /* BAUD = (4 * F_CPU) / baud  (normal async, S=16) */
    u->BAUD  = (uint16_t)((4UL * cfg->f_cpu_hz) / cfg->baud);

    /* 8-N-1 asynchronous */
    u->CTRLC = USART_CMODE_ASYNCHRONOUS_gc
             | USART_PMODE_DISABLED_gc
             | USART_SBMODE_1BIT_gc
             | USART_CHSIZE_8BIT_gc;

    /* Enable transmitter */
    u->CTRLB = USART_TXEN_bm;
}

void UART_SendByte(uart_instance_t inst, uint8_t byte)
{
    USART_t *u = prv_inst(inst);
    while (!(u->STATUS & USART_DREIF_bm)) {}
    u->TXDATAL = byte;
}

void UART_SendStr(uart_instance_t inst, const char *str)
{
    while (*str) {
        UART_SendByte(inst, (uint8_t)*str++);
    }
}

void UART_SendU16(uart_instance_t inst, uint16_t val)
{
    char     buf[6];
    uint8_t  i = 0;

    if (val == 0) {
        buf[i++] = '0';
    } else {
        uint16_t tmp = val;
        while (tmp) {
            buf[i++] = (char)('0' + (tmp % 10));
            tmp /= 10;
        }
        /* reverse */
        uint8_t lo = 0, hi = (uint8_t)(i - 1);
        while (lo < hi) {
            char t = buf[lo]; buf[lo++] = buf[hi]; buf[hi--] = t;
        }
    }
    buf[i] = '\0';
    UART_SendStr(inst, buf);
}

void UART_SendU32(uart_instance_t inst, uint32_t val)
{
    char    buf[11];
    uint8_t i = 0;

    if (val == 0) {
        buf[i++] = '0';
    } else {
        uint32_t tmp = val;
        while (tmp) {
            buf[i++] = (char)('0' + (uint8_t)(tmp % 10));
            tmp /= 10;
        }
        uint8_t lo = 0, hi = (uint8_t)(i - 1);
        while (lo < hi) {
            char t = buf[lo]; buf[lo++] = buf[hi]; buf[hi--] = t;
        }
    }
    buf[i] = '\0';
    UART_SendStr(inst, buf);
}

void UART_SendHex8(uart_instance_t inst, uint8_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    UART_SendByte(inst, (uint8_t)hex[val >> 4]);
    UART_SendByte(inst, (uint8_t)hex[val & 0x0F]);
}
