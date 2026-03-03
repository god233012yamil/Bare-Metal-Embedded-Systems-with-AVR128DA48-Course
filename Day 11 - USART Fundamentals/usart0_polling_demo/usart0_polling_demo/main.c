/*
 * AVR128DA48 USART0 Polling Demonstration
 * 
 * This project demonstrates basic USART0 communication using polling method.
 * 
 * Hardware:
 * - AVR128DA48 Curiosity Nano Board
 * - USART0: PA0 (TxD), PA1 (RxD)
 * 
 * Configuration:
 * - Baud Rate: 9600
 * - Data Bits: 8
 * - Parity: None
 * - Stop Bits: 1
 * - Clock: 4 MHz (default internal oscillator)
 *
 * Created with Microchip Studio 7
 * Device Pack: AVR-Dx 2.4.286
 */

#define F_CPU 4000000UL  // 4 MHz default clock frequency

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

// USART0 Baud Rate Calculation
// BAUD = 64 * f_CLK / (S * BAUD_RATE)
// For normal mode (S=16): BAUD = 64 * 4000000 / (16 * 9600) = 1667 ? 0x0683
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(64 * F_CPU / (16 * (float)BAUD_RATE)) + 0.5)

/**
 * @brief Initialize USART0 for asynchronous communication
 * 
 * This function configures USART0 with the following parameters:
 * - Baud rate: 9600 bps
 * - Character size: 8 bits
 * - Parity: None
 * - Stop bits: 1
 * - Mode: Asynchronous
 * 
 * Pin Configuration:
 * - PA0: TxD (Transmit Data) - Configured as output
 * - PA1: RxD (Receive Data) - Configured as input
 * 
 * @param None
 * @return None
 */
void USART0_init(void)
{
    // Set baud rate to 9600 bps
    // The baud rate register is 16-bit
    USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
    
    // Configure USART0 Control B register
    // Enable transmitter and receiver
    USART0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
    
    // Configure frame format: 8 data bits, no parity, 1 stop bit
    // This is actually the default, but shown here for clarity
    USART0.CTRLC = USART_CHSIZE_8BIT_gc  // 8-bit character size
                 | USART_PMODE_DISABLED_gc  // No parity
                 | USART_SBMODE_1BIT_gc;    // 1 stop bit
    
    // Configure pins
    // PA0 (TxD) as output
    PORTA.DIRSET = PIN0_bm;
    // PA1 (RxD) is input by default, but we'll explicitly configure it
    PORTA.DIRCLR = PIN1_bm;
}

/**
 * @brief Transmit a single character via USART0 (polling method)
 * 
 * This function sends one byte of data through USART0 using the polling method.
 * It waits until the transmit buffer is empty before writing the data.
 * 
 * The function blocks until the USART Data Register Empty (DREIF) flag is set,
 * indicating that the transmit buffer is ready to accept new data.
 * 
 * @param data The character/byte to transmit
 * @return None
 */
void USART0_sendChar(char data)
{
    // Wait for the transmit buffer to be empty
    // DREIF flag is set when the transmit buffer is ready
    while (!(USART0.STATUS & USART_DREIF_bm))
    {
        ; // Busy wait
    }
    
    // Write data to the transmit buffer
    USART0.TXDATAL = data;
}

/**
 * @brief Transmit a null-terminated string via USART0 (polling method)
 * 
 * This function sends a complete string through USART0 by repeatedly calling
 * USART0_sendChar() for each character in the string.
 * 
 * The function will transmit all characters until it encounters the null
 * terminator ('\0').
 * 
 * @param str Pointer to the null-terminated string to transmit
 * @return None
 */
void USART0_sendString(const char *str)
{
    // Transmit each character until null terminator is reached
    while (*str)
    {
        USART0_sendChar(*str++);
    }
}

/**
 * @brief Receive a single character via USART0 (polling method)
 * 
 * This function receives one byte of data from USART0 using the polling method.
 * It waits until data is available in the receive buffer before reading it.
 * 
 * The function blocks until the Receive Complete Interrupt Flag (RXCIF) is set,
 * indicating that unread data is available in the receive buffer.
 * 
 * @param None
 * @return The received character/byte
 */
char USART0_receiveChar(void)
{
    // Wait for data to be received
    // RXCIF flag is set when unread data exists in the receive buffer
    while (!(USART0.STATUS & USART_RXCIF_bm))
    {
        ; // Busy wait
    }
    
    // Read and return received data
    return USART0.RXDATAL;
}

/**
 * @brief Check if data is available in the USART0 receive buffer
 * 
 * This function checks the Receive Complete Interrupt Flag (RXCIF) without
 * blocking. It can be used for non-blocking receive operations.
 * 
 * @param None
 * @return 1 if data is available, 0 otherwise
 */
uint8_t USART0_isDataAvailable(void)
{
    // Check if the receive complete flag is set
    return (USART0.STATUS & USART_RXCIF_bm) ? 1 : 0;
}

/**
 * @brief Main application entry point
 * 
 * This function demonstrates USART0 communication using polling method:
 * 1. Initializes USART0
 * 2. Sends a welcome message
 * 3. Enters an echo loop that receives characters and echoes them back
 * 
 * Echo Loop Behavior:
 * - Receives a character from the serial terminal
 * - Echoes the character back to the terminal
 * - Continues indefinitely
 * 
 * @param None
 * @return int (never returns in this implementation)
 */
int main(void)
{
    char received_char;
    
    // Initialize USART0
    USART0_init();
    
    // Small delay to ensure USART is ready
    _delay_ms(100);
    
    // Send welcome message
    USART0_sendString("\r\n");
    USART0_sendString("===================================\r\n");
    USART0_sendString("AVR128DA48 USART0 Polling Demo\r\n");
    USART0_sendString("===================================\r\n");
    USART0_sendString("Echo Mode Active\r\n");
    USART0_sendString("Type any character to echo it back\r\n");
    USART0_sendString("-----------------------------------\r\n\r\n");
    
    // Main loop - Echo received characters
    while (1)
    {
        // Wait for and receive a character
        received_char = USART0_receiveChar();
        
        // Echo the character back
        USART0_sendChar(received_char);
        
        // Optional: Add line feed if carriage return is received
        if (received_char == '\r')
        {
            USART0_sendChar('\n');
        }
    }
    
    return 0;  // Never reached
}