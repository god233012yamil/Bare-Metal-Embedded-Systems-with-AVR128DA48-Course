/**
 * @file    usart.c
 * @brief   USART0 driver — implementation.
 *
 * See usart.h for the public API and design rationale.
 *
 * Register-level notes (AVR128DA48 data sheet, §26):
 *  - PORTMUX.USARTROUTEA selects the pin mapping for each USART.
 *    USART0 default: TXD = PA0, RXD = PA1 (CDC-USB bridge on Curiosity Nano).
 *  - USART0.BAUD  is a 16-bit register: BAUD = (64 * F_CPU) / (16 * baud).
 *  - USART0.CTRLB enables TX (TXEN) and RX (RXEN) independently.
 *  - USART0.CTRLC sets the frame format (8N1 = default after reset).
 *  - USART0.CTRLA enables the RXCIF interrupt (RXCIE bit).
 *  - The TX data register is USART0.TXDATAL (8-bit write).
 *  - The RX data register is USART0.RXDATAL (8-bit read, clears RXCIF).
 *  - STATUS.DREIF is set when the TX data register is empty (ready to load).
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "usart.h"
#include "fifo.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

/* =========================================================================
 * Module-private state
 * ========================================================================= */

/** Backing storage for the RX ring buffer. */
static uint8_t  g_rxBuf[FIFO_RX_SIZE];

/** RX ring-buffer control structure. */
static Fifo_t   g_rxFifo;

/* =========================================================================
 * ISR
 * ========================================================================= */

/**
 * @brief USART0 Receive Complete ISR.
 *
 * Reads RXDATAL (clears the RXCIF hardware flag) and deposits the byte
 * into the RX FIFO.  If the FIFO is full the byte is silently discarded;
 * the application should drain the FIFO fast enough to prevent overflow.
 */
ISR(USART0_RXC_vect)
{
    uint8_t data = USART0.RXDATAL;     /* reading clears RXCIF */
    (void)FIFO_PutByte(&g_rxFifo, data);
}

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises USART0 for 115200 8N1 with interrupt-driven RX.
 */
void USART_Init(void)
{
    /* ---- Initialise the RX FIFO ---- */
    FIFO_Init(&g_rxFifo, g_rxBuf, FIFO_RX_SIZE);

    /* ---- Pin configuration ---- */
    /* Route USART0 to default pins (PA0 = TXD, PA1 = RXD). */
    PORTMUX.USARTROUTEA = PORTMUX_USART0_DEFAULT_gc;

    /* Set PA0 as output (TXD); PA1 defaults to input (RXD). */
    PORTA.DIRSET = USART_TX_PIN_bm;

    /* Drive TXD high before enabling the transmitter to avoid a spurious
     * start bit. */
    PORTA.OUTSET = USART_TX_PIN_bm;

    /* ---- Baud rate ---- */
    USART0.BAUD = USART_BAUD_REG_VALUE;

    /* ---- Frame format: 8N1 (reset default; written explicitly for clarity)---- */
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc
                 | USART_PMODE_DISABLED_gc
                 | USART_SBMODE_1BIT_gc
                 | USART_CHSIZE_8BIT_gc;

    /* ---- Enable transmitter and receiver ---- */
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;

    /* ---- Enable the RXCIF (Receive Complete) interrupt ---- */
    USART0.CTRLA = USART_RXCIE_bm;
}

/**
 * @brief Transmits one byte, blocking until the data register is empty.
 *
 * @param[in] byte  Byte to transmit.
 */
void USART_SendByte(uint8_t byte)
{
    /* Wait for the Data Register Empty flag. */
    while (!(USART0.STATUS & USART_DREIF_bm)) { /* spin */ }
    USART0.TXDATAL = byte;
}

/**
 * @brief Transmits a null-terminated string.
 *
 * @param[in] str  Null-terminated C string.
 */
void USART_SendString(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        USART_SendByte((uint8_t)*str);
        str++;
    }
}

/**
 * @brief Transmits a uint16_t as decimal ASCII (no leading zeros).
 *
 * @param[in] value  Value to print (0–65535).
 */
void USART_SendUInt16(uint16_t value)
{
    char     buf[6];    /* max 5 digits + null */
    uint8_t  idx = 5U;
    uint8_t  start;

    buf[idx] = '\0';

    if (value == 0U)
    {
        USART_SendByte('0');
        return;
    }

    while (value > 0U)
    {
        idx--;
        buf[idx] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    start = idx;
    while (buf[start] != '\0')
    {
        USART_SendByte((uint8_t)buf[start]);
        start++;
    }
}

/**
 * @brief Attempts to dequeue one byte from the RX FIFO.
 *
 * @param[out] byte  Receives the byte on success.
 * @return true if a byte was available.
 */
bool USART_GetByte(uint8_t *byte)
{
    return FIFO_GetByte(&g_rxFifo, byte);
}

/**
 * @brief Reports whether the RX FIFO is non-empty.
 *
 * @return true if data is waiting.
 */
bool USART_RxAvailable(void)
{
    return !FIFO_IsEmpty(&g_rxFifo);
}
