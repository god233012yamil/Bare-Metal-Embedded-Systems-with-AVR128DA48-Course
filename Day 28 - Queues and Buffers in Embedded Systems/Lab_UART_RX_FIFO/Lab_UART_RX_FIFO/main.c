/**
 * @file main.c
 * @brief Lab – UART RX FIFO Buffer: application entry point and main loop.
 *
 * Demonstrates interrupt-driven UART reception on the AVR128DA48 Curiosity
 * Nano board. The program:
 *
 *  1. Initialises USART1 (9600 8-N-1) with an interrupt-driven receive path
 *     backed by a 64-byte circular FIFO.
 *  2. Sends a startup banner over the UART so the user can verify TX works.
 *  3. Enters an infinite loop that processes bytes from the FIFO one at a
 *     time:
 *       - Each received byte is echoed back to the sender.
 *       - Printable ASCII characters are also announced as a formatted message.
 *       - A newline ('\n' / '\r') triggers a prompt message.
 *       - FIFO occupancy is printed whenever data is being processed.
 *
 * Hardware connections (Curiosity Nano, default USART1 MUX):
 *   PC0  ?  TXD  (to debugger CDC UART RX)
 *   PC1  ?  RXD  (from debugger CDC UART TX)
 *
 * Open a serial terminal at 9600 baud, 8-N-1, no flow control, connected to
 * the Curiosity Nano's virtual COM port to interact with this firmware.
 *
 * @note F_CPU is defined in the Atmel Studio project properties
 *       (Project ? Properties ? Toolchain ? AVR/GNU C Compiler ? Symbols).
 *       The default internal oscillator runs at 4 MHz; match F_CPU accordingly.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>          /* sprintf()  */
#include "uart.h"
#include "fifo.h"

/* =========================================================================
 * Private helper prototypes
 * ====================================================================== */

static void process_byte(uint8_t byte);
static void print_fifo_status(void);

/* =========================================================================
 * Entry point
 * ====================================================================== */

/**
 * @brief Application entry point.
 *
 * Initializes peripherals and enters the super-loop. The UART RX interrupt
 * handles incoming bytes asynchronously; the main loop drains and processes
 * the FIFO.
 *
 * @return This function never returns (embedded main loop).
 */
int main(void)
{
    /* -----------------------------------------------------------------
     * Peripheral initialization
     * -------------------------------------------------------------- */

    /* Initialize USART1: sets up baud rate, pins, enables RX interrupt,
     * calls sei() to enable global interrupts.                          */
    UART_Init();

    /* -----------------------------------------------------------------
     * Startup banner – confirms TX path is working
     * -------------------------------------------------------------- */
    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  AVR128DA48 – UART RX FIFO Buffer Lab \r\n");
    UART_SendString("  USART1 @ 9600 baud, 8-N-1            \r\n");
    UART_SendString("  FIFO size: 64 bytes                  \r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("Type characters and press Enter...\r\n\r\n");

    /* -----------------------------------------------------------------
     * Super-loop
     * -------------------------------------------------------------- */
    while (1)
    {
        /* Check the FIFO; process one byte per iteration so the loop
         * remains responsive and no single character monopolises CPU.   */
        if (UART_ByteAvailable())
        {
            uint8_t received;

            if (UART_GetByte(&received))
            {
                /* Show current FIFO occupancy before processing. */
                print_fifo_status();

                /* Handle the received byte. */
                process_byte(received);
            }
        }

        /* Other application tasks would go here. The main loop stays
         * unblocked because UART RX is fully interrupt-driven.          */
    }

    /* Unreachable on AVR, but satisfies the C standard. */
    return 0;
}

/* =========================================================================
 * Private helpers
 * ====================================================================== */

/**
 * @brief Process a single byte received from the UART.
 *
 * Echoes the byte back, then prints a human-readable description:
 *   - Printable ASCII (0x20–0x7E): announces the character.
 *   - Carriage return (0x0D) or line feed (0x0A): prints a prompt.
 *   - All other values: prints the raw hex value.
 *
 * @param byte  The byte to process.
 */
static void process_byte(uint8_t byte)
{
    char msg[48];   /* Scratch buffer for formatted output. */

    /* 1. Echo the raw byte back to the terminal. */
    UART_SendByte(byte);

    /* 2. Describe what was received. */
    if (byte >= 0x20u && byte <= 0x7Eu)
    {
        /* Printable ASCII character. */
        sprintf(msg, " <- received printable: '%c' (0x%02X)\r\n",
                (char)byte, byte);
        UART_SendString(msg);
    }
    else if (byte == '\r' || byte == '\n')
    {
        /* Line ending – print a fresh prompt. */
        UART_SendString("\r\n> ");
    }
    else
    {
        /* Non-printable / control character. */
        sprintf(msg, " <- received control: 0x%02X\r\n", byte);
        UART_SendString(msg);
    }
}

/**
 * @brief Print the current FIFO occupancy to the UART.
 *
 * Outputs a line of the form:
 *   [FIFO] bytes in buffer: N / 63
 *
 * The maximum is 63 (not 64) because one slot is always reserved to
 * distinguish a full buffer from an empty one in the circular design.
 *
 * @note Calling FIFO_Count() from the main loop while the ISR may
 *       concurrently increment uart_rx_fifo.head is safe on AVR:
 *       head is a volatile uint8_t, and 8-bit reads are atomic on AVR.
 */
static void print_fifo_status(void)
{
    char msg[40];
    uint8_t count = FIFO_Count(&uart_rx_fifo);

    sprintf(msg, "[FIFO] bytes in buffer: %u / %u\r\n",
            (unsigned)count,
            (unsigned)(FIFO_BUFFER_SIZE - 1u));

    UART_SendString(msg);
}