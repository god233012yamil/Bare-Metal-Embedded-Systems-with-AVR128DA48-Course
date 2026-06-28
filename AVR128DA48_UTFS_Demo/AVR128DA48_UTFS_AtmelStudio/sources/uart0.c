#include "uart0.h"

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes USART0 for 8N1 serial communication.
 *
 * Args:
 *     peripheral_clock_hz: CLK_PER frequency supplied to USART0.
 *     baudrate: Desired UART baud rate. The demo uses 115200 by default.
 */
bool uart0_init(uint32_t peripheral_clock_hz, uint32_t baudrate)
{
   uint32_t baud_register;
   uint8_t rx_mode;

   if ((peripheral_clock_hz == 0u) || (baudrate == 0u)) {
       return false;
   }

   /* BAUD = 64 * f_CLK_PER / (S * baudrate), where S is 16 in
    * normal mode and 8 in CLK2X mode. At 1.2 MHz and 115200 baud,
    * normal mode is too fast for 16x sampling, so use CLK2X. */
   baud_register = ((peripheral_clock_hz * 4u) + (baudrate / 2u)) / baudrate;
   rx_mode = USART_RXMODE_NORMAL_gc;

   if (baud_register < 64u) {
       baud_register = ((peripheral_clock_hz * 8u) + (baudrate / 2u)) / baudrate;
       rx_mode = USART_RXMODE_CLK2X_gc;
   }

   if ((baud_register < 64u) || (baud_register > UINT16_MAX)) {
       return false;
   }

   // Define the pins used by the UART0 peripheral
   PORTA.DIRSET = PIN0_bm;
   PORTA.DIRCLR = PIN1_bm;

   USART0.BAUD = (uint16_t)baud_register;

   USART0.CTRLA = 0 << USART_ABEIE_bp /* Auto-baud Error Interrupt Enable: disabled */
	   | 0 << USART_DREIE_bp /* Data Register Empty Interrupt Enable: disabled */
	   | 0 << USART_LBME_bp /* Loop-back Mode Enable: disabled */
	   | USART_RS485_DISABLE_gc /* RS485 Mode disabled */
	   | 0 << USART_RXCIE_bp /* Receive Complete Interrupt Enable: disabled */
	   | 0 << USART_RXSIE_bp /* Receiver Start Frame Interrupt Enable: disabled */
	   | 0 << USART_TXCIE_bp; /* Transmit Complete Interrupt Enable: disabled */

   USART0.CTRLB = 0 << USART_MPCM_bp       /* Multi-processor Communication Mode: disabled */
	   | 0 << USART_ODME_bp     /* Open Drain Mode Enable: disabled */
	   | 1 << USART_RXEN_bp     /* Receiver Enable: enabled */
	   | rx_mode              /* Normal or double-speed mode */
	   | 0 << USART_SFDEN_bp    /* Start Frame Detection Enable: disabled */
	   | 1 << USART_TXEN_bp;    /* Transmitter Enable: enabled */

   USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc /* Asynchronous Mode */
	   | USART_CHSIZE_8BIT_gc /* Character size: 8 bit */
	   | USART_PMODE_DISABLED_gc /* No Parity */
	   | USART_SBMODE_1BIT_gc; /* 1 stop bit */

   USART0.DBGCTRL = 0 << USART_DBGRUN_bp; /* Debug Run: disabled */

   USART0.EVCTRL = 0 << USART_IREI_bp; /* IrDA Event Input Enable: disabled */

   USART0.RXPLCTRL = 0x0 << USART_RXPL_gp; /* Receiver Pulse Length: 0x0 */

   USART0.TXPLCTRL = 0x0 << USART_TXPL_gp; /* Transmit pulse length: 0x0 */

   return true;
}

/**
 * Writes one character to USART0.
 *
 * Args:
 *     c: Character to transmit.
 */
void uart0_write_char(char c)
{
    while ((USART0.STATUS & USART_DREIF_bm) == 0u) {
        ;
    }

    USART0.TXDATAL = (uint8_t)c;
}

/**
 * Writes a null-terminated string to USART0.
 *
 * Args:
 *     text: String to transmit.
 */
void uart0_write_string(const char *text)
{
    while (*text != '\0') {
        uart0_write_char(*text++);
    }
}

/**
 * Attempts to read one character from USART0 without blocking.
 *
 * Args:
 *     c: Pointer that receives the character when one is available.
 *
 * Returns:
 *     true when a character was read, otherwise false.
 */
bool uart0_read_char_nonblocking(char *c)
{
    if ((USART0.STATUS & USART_RXCIF_bm) == 0u) {
        return false;
    }

    *c = (char)USART0.RXDATAL;
    return true;
}
