/**
 * @file main.c
 * @brief Minimal interrupt-driven USART0 example for AVR128DA48.
 *
 * This example demonstrates:
 * - USART0 configured in asynchronous 8N1 mode.
 * - Interrupt-driven RX using a ring buffer.
 * - Interrupt-driven TX using a ring buffer.
 * - Non-blocking main loop.
 * - Simple command interface: send 't' to toggle LED.
 *
 * Hardware assumptions:
 * - F_CPU = 4 MHz
 * - USART0 TX on PA0
 * - USART0 RX on PA1
 * - LED on PA2
 *
 * Adjust pin definitions as required for your board routing.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* ============================= */
/* Configuration Macros          */
/* ============================= */

#define F_CPU_HZ    (4000000UL)
#define BAUD_RATE   (9600UL)

/*
 * BAUD calculation for AVR DA asynchronous normal mode.
 * See datasheet for exact formula.
 */
#define USART0_BAUD_VALUE  ((uint16_t)((F_CPU_HZ * 64UL) / (16UL * BAUD_RATE)))

#define UART_RX_BUFFER_SIZE   64
#define UART_TX_BUFFER_SIZE   64

/* ============================= */
/* Global Buffers and Indexes    */
/* ============================= */

static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t tx_buffer[UART_TX_BUFFER_SIZE];

static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

/* ============================= */
/* Function Prototypes           */
/* ============================= */

static void gpio_init(void);
static void usart0_init(void);
static void usart0_write(uint8_t data);
static void usart0_write_string(const char *str);
static uint8_t usart0_available(void);
static uint8_t usart0_read(void);

/* ============================= */
/* GPIO Initialization           */
/* ============================= */

/**
 * @brief Initializes GPIO pins for LED and USART.
 *
 * Configures:
 * - PA2 as LED output.
 * - PA0 as USART TX output.
 * - PA1 as USART RX input.
 */
static void gpio_init(void)
{
    /* LED on PA2 */
    PORTA.DIRSET = PIN2_bm;
    PORTA.OUTCLR = PIN2_bm;

    /* USART0 TX (PA0) as output */
    PORTA.DIRSET = PIN0_bm;

    /* USART0 RX (PA1) as input */
    PORTA.DIRCLR = PIN1_bm;
}

/* ============================= */
/* USART Initialization          */
/* ============================= */

/**
 * @brief Initializes USART0 in asynchronous 8N1 mode with interrupts.
 *
 * Configures:
 * - Baud rate to BAUD_RATE.
 * - 8 data bits, no parity, 1 stop bit.
 * - RX interrupt enabled.
 * - TX and RX enabled.
 */
static void usart0_init(void)
{
    USART0.BAUD = USART0_BAUD_VALUE;

    /* 8-bit character size */
    USART0.CTRLC = USART_CHSIZE_8BIT_gc;

    /* Enable RX interrupt */
    USART0.CTRLA = USART_RXCIE_bm;

    /* Enable TX and RX */
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
}

/* ============================= */
/* USART TX (Non-Blocking)       */
/* ============================= */

/**
 * @brief Queues a byte for transmission using interrupt-driven TX.
 *
 * @param data Byte to transmit.
 *
 * Blocks only if the TX buffer is full.
 */
static void usart0_write(uint8_t data)
{
    uint8_t next_head = (tx_head + 1) % UART_TX_BUFFER_SIZE;

    /* Wait if buffer full */
    while (next_head == tx_tail)
    {
        /* Optional: add timeout or error handling */
    }

    tx_buffer[tx_head] = data;
    tx_head = next_head;

    /* Enable Data Register Empty interrupt */
    USART0.CTRLA |= USART_DREIE_bm;
}

/**
 * @brief Queues a null-terminated string for transmission.
 *
 * @param str Pointer to string.
 */
static void usart0_write_string(const char *str)
{
    while (*str)
    {
        usart0_write((uint8_t)(*str));
        str++;
    }
}

/* ============================= */
/* USART RX (Non-Blocking)       */
/* ============================= */

/**
 * @brief Returns non-zero if data is available in RX buffer.
 *
 * @return 1 if data available, 0 otherwise.
 */
static uint8_t usart0_available(void)
{
    return (rx_head != rx_tail);
}

/**
 * @brief Reads one byte from RX buffer.
 *
 * @return Received byte.
 *
 * Caller must ensure data is available before calling.
 */
static uint8_t usart0_read(void)
{
    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
    return data;
}

/* ============================= */
/* USART Interrupt Handlers      */
/* ============================= */

/**
 * @brief USART0 RX Complete ISR.
 *
 * Triggered when a byte is received.
 * Stores received byte in RX ring buffer.
 */
ISR(USART0_RXC_vect)
{
    uint8_t data = USART0.RXDATAL;
    uint8_t next_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;

    if (next_head != rx_tail)
    {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
    /* else: buffer overflow, byte discarded */
}

/**
 * @brief USART0 Data Register Empty ISR.
 *
 * Triggered when TX register is ready for new data.
 * Sends next byte from TX buffer.
 */
ISR(USART0_DRE_vect)
{
    if (tx_head == tx_tail)
    {
        /* Buffer empty, disable interrupt */
        USART0.CTRLA &= ~USART_DREIE_bm;
    }
    else
    {
        USART0.TXDATAL = tx_buffer[tx_tail];
        tx_tail = (tx_tail + 1) % UART_TX_BUFFER_SIZE;
    }
}

/* ============================= */
/* Main Application              */
/* ============================= */

/**
 * @brief Main entry point.
 *
 * Initializes GPIO and USART.
 * Enables global interrupts.
 * Implements a simple command interface:
 * - Send 't' to toggle LED.
 */
int main(void)
{
    gpio_init();
    usart0_init();

    sei();

    usart0_write_string("USART Interrupt Example Ready\r\n");

    while (1)
    {
        if (usart0_available())
        {
            uint8_t c = usart0_read();

            if (c == 't')
            {
                PORTA.OUTTGL = PIN2_bm;
                usart0_write_string("LED toggled\r\n");
            }
        }
    }
}