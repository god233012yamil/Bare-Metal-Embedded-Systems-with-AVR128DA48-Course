/**
 * @file uart.c
 * @brief UART driver implementation for the AVR128DA48, using USART1 (PC0/PC1).
 *
 * Implements interrupt-driven receive via the USART1 RXC (Receive Complete)
 * interrupt. Each received byte is pushed into uart_rx_fifo immediately inside
 * the ISR. The main application loop drains the FIFO through UART_GetByte().
 *
 * Transmit is polled (blocking) – suitable for modest data rates and debug
 * output where simplicity matters more than transmit throughput.
 *
 * Register names and bit-field macros are taken from the Microchip AVR-Dx
 * Device Pack (ioavr128da48.h) included by <avr/io.h>.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "fifo.h"

/* =========================================================================
 * Global FIFO instance
 * ====================================================================== */

/**
 * @brief Receive FIFO shared between the RXC ISR (writer) and the main loop
 *        (reader). Declared extern in uart.h so the ISR can remain in this
 *        translation unit without requiring a function-call overhead.
 */
FIFO_t uart_rx_fifo;

/* =========================================================================
 * Public API implementation
 * ====================================================================== */

/**
 * @brief Initialise USART1 for 8-N-1 operation at UART_BAUD_RATE.
 *
 * Steps performed:
 *  1. Initialise the receive FIFO.
 *  2. Configure PC0 as output (TXD) and PC1 as input (RXD).
 *  3. Set the baud rate register.
 *  4. Enable the transmitter, receiver, and RXC interrupt.
 *  5. Enable global interrupts.
 *
 * @note The USART1 MUX defaults to PC0/PC1 after reset, so no PORTMUX
 *       register changes are required for the Curiosity Nano board.
 */
void UART_Init(void)
{
    /* 1. Prepare the receive FIFO before enabling the interrupt. */
    FIFO_Init(&uart_rx_fifo);

    /* 2. Pin directions.
     *    PC0 = TXD: drive the line; set output and idle-high. */
    PORTC.DIRSET  = PIN0_bm;            /* PC0 output (TXD)              */
    PORTC.OUTSET  = PIN0_bm;            /* idle-high (UART idle state)   */
    PORTC.DIRCLR  = PIN1_bm;            /* PC1 input  (RXD)              */

    /* 3. Baud rate.
     *    UART_BAUD_VALUE is computed at compile time in uart.h. */
    USART1.BAUD   = UART_BAUD_VALUE;

    /* 4. Frame format: 8 data bits, no parity, 1 stop bit (reset default).
     *    CTRLC defaults to 0x03 (CHSIZE=8) after reset, so no write needed.
     *    Enable TX, RX, and the RXC interrupt. */
    USART1.CTRLB  = USART_TXEN_bm       /* transmitter enable            */
                  | USART_RXEN_bm;      /* receiver enable               */

    USART1.CTRLA  = USART_RXCIE_bm;     /* RX Complete Interrupt enable  */

    /* 5. Enable global interrupts so the RXC ISR can fire. */
    sei();
}

/**
 * @brief Blocking single-byte transmit.
 *
 * Waits for the Data Register Empty Flag (DREIF) to indicate the hardware
 * transmit buffer is ready, then writes the byte. The function returns as
 * soon as the byte is accepted; the byte may still be shifting out serially
 * when the function returns.
 *
 * @param data  Byte to transmit.
 */
void UART_SendByte(uint8_t data)
{
    /* Wait until the transmit data register is empty. */
    while (!(USART1.STATUS & USART_DREIF_bm))
    {
        /* busy-wait */
    }

    USART1.TXDATAL = data;
}

/**
 * @brief Blocking string transmit.
 *
 * Iterates over the string, transmitting one character at a time via
 * UART_SendByte(). Stops at the null terminator (which is not sent).
 *
 * @param str  Pointer to a null-terminated string.
 */
void UART_SendString(const char *str)
{
    while (*str != '\0')
    {
        UART_SendByte((uint8_t)*str);
        str++;
    }
}

/**
 * @brief Non-blocking read from the receive FIFO.
 *
 * Delegates directly to FIFO_Get(). Interrupt-safety is inherent because
 * FIFO_Get() reads tail (consumer side only) and the ISR writes head
 * (producer side only); no critical section is required.
 *
 * @param data  Output pointer for the received byte.
 * @return true  Byte available and stored in *data.
 * @return false FIFO empty.
 */
bool UART_GetByte(uint8_t *data)
{
    return FIFO_Get(&uart_rx_fifo, data);
}

/**
 * @brief Query whether at least one byte is waiting in the receive FIFO.
 *
 * @return true  Data available.
 * @return false FIFO empty.
 */
bool UART_ByteAvailable(void)
{
    return !FIFO_IsEmpty(&uart_rx_fifo);
}

/* =========================================================================
 * Interrupt Service Routine
 * ====================================================================== */

/**
 * @brief USART1 Receive Complete interrupt handler.
 *
 * Fires each time the USART1 hardware receive buffer holds a new byte. The
 * handler reads the byte from RXDATAL (which also clears the RXCIF flag) and
 * pushes it into uart_rx_fifo.
 *
 * If the FIFO is full the byte is silently discarded; the application must
 * drain the FIFO faster than bytes arrive to avoid data loss.
 *
 * @note Reading RXDATAL before checking error flags is intentional: on
 *       AVR-Dx the RXDATAL read clears RXCIF, so it must always be read to
 *       allow the next interrupt to fire. Error flags (FERR, PERR, BUFOVF)
 *       can be added here if protocol-level error handling is required.
 */
ISR(USART1_RXC_vect)
{
    /* Read the received byte – this also clears the RXCIF flag. */
    uint8_t received_byte = USART1.RXDATAL;

    /* Push into the FIFO; overflow is silently handled inside FIFO_Put(). */
    FIFO_Put(&uart_rx_fifo, received_byte);
}
