/*
 * AVR128DA48 USART0 Interrupt-Driven Demonstration
 * 
 * This project demonstrates advanced USART0 communication using:
 * - Interrupt-driven TX and RX (non-blocking)
 * - Ring buffers for efficient data handling
 * - Multiple example use cases
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
 * Features Demonstrated:
 * - Non-blocking serial communication
 * - Command processing
 * - printf-style formatting
 * - Background data transmission/reception
 * - Ring buffer usage
 *
 * Created with Microchip Studio 7
 * Device Pack: AVR-Dx 2.4.286
 */

#define F_CPU 4000000UL  // 4 MHz default clock frequency

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <ctype.h>
#include "usart0_interrupt.h"

// Command buffer for receiving user input
#define CMD_BUFFER_SIZE 64
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_index = 0;

// LED control (using onboard LED if available, or define your own pin)
#define LED_PIN PIN6_bm
#define LED_PORT PORTC

/**
 * @brief Initialize the onboard LED
 * 
 * Configures the LED pin as an output and turns it off initially.
 * On the AVR128DA48 Curiosity Nano, the onboard LED is typically on PC6.
 * Adjust LED_PIN and LED_PORT if using a different pin.
 * 
 * @param None
 * @return None
 */
void LED_init(void)
{
    // Set LED pin as output
    LED_PORT.DIRSET = LED_PIN;
    // Turn LED off initially
    LED_PORT.OUTCLR = LED_PIN;
}

/**
 * @brief Turn LED on
 * 
 * @param None
 * @return None
 */
void LED_on(void)
{
    LED_PORT.OUTSET = LED_PIN;
}

/**
 * @brief Turn LED off
 * 
 * @param None
 * @return None
 */
void LED_off(void)
{
    LED_PORT.OUTCLR = LED_PIN;
}

/**
 * @brief Toggle LED state
 * 
 * @param None
 * @return None
 */
void LED_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN;
}

/**
 * @brief Process received commands
 * 
 * This function parses and executes commands received via USART0.
 * 
 * Supported commands:
 * - "help" or "?" : Display help menu
 * - "led on"      : Turn LED on
 * - "led off"     : Turn LED off
 * - "led toggle"  : Toggle LED state
 * - "status"      : Display system status
 * - "echo <text>" : Echo back the text
 * - "clear"       : Clear receive buffer
 * 
 * @param cmd Pointer to null-terminated command string
 * @return None
 */
void process_command(char *cmd)
{
    // Convert command to lowercase for case-insensitive comparison
    for (char *p = cmd; *p; p++) {
        *p = tolower(*p);
    }
    
    // Remove trailing newline/carriage return
    char *newline = strchr(cmd, '\r');
    if (newline) *newline = '\0';
    newline = strchr(cmd, '\n');
    if (newline) *newline = '\0';
    
    // Process commands
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        USART0_writeString_blocking("\r\n=== Available Commands ===\r\n");
        USART0_writeString_blocking("  help or ?      - Show this help menu\r\n");
        USART0_writeString_blocking("  led on         - Turn LED on\r\n");
        USART0_writeString_blocking("  led off        - Turn LED off\r\n");
        USART0_writeString_blocking("  led toggle     - Toggle LED state\r\n");
        USART0_writeString_blocking("  status         - Show system status\r\n");
        USART0_writeString_blocking("  echo <text>    - Echo back text\r\n");
        USART0_writeString_blocking("  clear          - Clear RX buffer\r\n");
        USART0_writeString_blocking("==========================\r\n\r\n");
    }
    else if (strcmp(cmd, "led on") == 0) {
        LED_on();
        USART0_writeString_blocking("LED turned ON\r\n");
    }
    else if (strcmp(cmd, "led off") == 0) {
        LED_off();
        USART0_writeString_blocking("LED turned OFF\r\n");
    }
    else if (strcmp(cmd, "led toggle") == 0) {
        LED_toggle();
        USART0_writeString_blocking("LED toggled\r\n");
    }
    else if (strcmp(cmd, "status") == 0) {
        USART0_writeString_blocking("\r\n=== System Status ===\r\n");
        USART0_printf("  CPU Clock: %lu Hz\r\n", (uint32_t)F_CPU);
        USART0_printf("  RX Buffer: %u bytes available\r\n", USART0_available());
        USART0_printf("  TX Buffer: %u bytes free\r\n", USART0_txFreeSpace());
        USART0_printf("  TX Complete: %s\r\n", USART0_txComplete() ? "Yes" : "No");
        USART0_writeString_blocking("=====================\r\n\r\n");
    }
    else if (strncmp(cmd, "echo ", 5) == 0) {
        USART0_writeString_blocking("Echo: ");
        USART0_writeString_blocking(cmd + 5);
        USART0_writeString_blocking("\r\n");
    }
    else if (strcmp(cmd, "clear") == 0) {
        USART0_flushRx();
        USART0_writeString_blocking("RX buffer cleared\r\n");
    }
    else if (strlen(cmd) > 0) {
        USART0_writeString_blocking("Unknown command: ");
        USART0_writeString_blocking(cmd);
        USART0_writeString_blocking("\r\nType 'help' for available commands\r\n");
    }
}

/**
 * @brief Display welcome message and instructions
 * 
 * Sends a formatted welcome banner to the serial terminal with
 * basic usage instructions.
 * 
 * @param None
 * @return None
 */
void display_welcome(void)
{
    USART0_writeString_blocking("\r\n\r\n");
    USART0_writeString_blocking("========================================\r\n");
    USART0_writeString_blocking("  AVR128DA48 Interrupt-Driven USART\r\n");
    USART0_writeString_blocking("  Ring Buffer Implementation\r\n");
    USART0_writeString_blocking("========================================\r\n");
    USART0_writeString_blocking("\r\n");
    USART0_printf("  Device: AVR128DA48\r\n");
    USART0_printf("  Clock: %lu Hz\r\n", (uint32_t)F_CPU);
    USART0_printf("  Baud Rate: 9600 bps\r\n");
    USART0_printf("  RX Buffer: %d bytes\r\n", USART0_RX_BUFFER_SIZE);
    USART0_printf("  TX Buffer: %d bytes\r\n", USART0_TX_BUFFER_SIZE);
    USART0_writeString_blocking("\r\n");
    USART0_writeString_blocking("Type 'help' for available commands\r\n");
    USART0_writeString_blocking("----------------------------------------\r\n");
    USART0_writeString_blocking("> ");
}

/**
 * @brief Handle received character in command mode
 * 
 * This function processes each received character and builds up a
 * command string until Enter is pressed.
 * 
 * Features:
 * - Local echo (characters are echoed back to terminal)
 * - Backspace support
 * - Command execution on Enter
 * - Buffer overflow protection
 * 
 * @param c Received character
 * @return None
 */
void handle_received_char(uint8_t c)
{
    // Echo character back for local terminal display
    USART0_write(c);
    
    // Handle special characters
    if (c == '\r' || c == '\n') {
        // End of command - process it
        USART0_writeString_blocking("\r\n");
        cmd_buffer[cmd_index] = '\0';
        process_command(cmd_buffer);
        cmd_index = 0;
        USART0_writeString_blocking("> ");
    }
    else if (c == 127 || c == '\b') {
        // Backspace - remove last character
        if (cmd_index > 0) {
            cmd_index--;
            USART0_writeString_blocking("\b \b");  // Erase character on terminal
        }
    }
    else if (c >= 32 && c < 127) {
        // Printable character - add to buffer
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
        } else {
            // Buffer full - notify user
            USART0_writeString_blocking("\r\n[Buffer full]\r\n> ");
            cmd_index = 0;
        }
    }
    // Ignore other non-printable characters
}

/**
 * @brief Demonstrate non-blocking background tasks
 * 
 * This function simulates work that can be done while USART communication
 * happens in the background via interrupts. The LED blinks to show the
 * CPU is free to do other work.
 * 
 * In a real application, this is where you would:
 * - Read sensors
 * - Update displays
 * - Process data
 * - Run control algorithms
 * - etc.
 * 
 * @param None
 * @return None
 */
void background_tasks(void)
{
    static uint32_t last_blink = 0;
    static uint32_t tick_count = 0;
    
    tick_count++;
    
    // Blink LED every ~500ms to show CPU is running
    // This demonstrates that the CPU is free while USART operates
    if (tick_count - last_blink > 50000) {
        LED_toggle();
        last_blink = tick_count;
    }
}

/**
 * @brief Main application entry point
 * 
 * Initializes the system and enters the main loop which:
 * 1. Checks for received data
 * 2. Processes commands
 * 3. Performs background tasks
 * 
 * The key advantage of interrupt-driven USART is demonstrated here:
 * The CPU can perform other tasks (background_tasks) while serial
 * communication happens automatically in the background.
 * 
 * @param None
 * @return int (never returns)
 */
int main(void)
{
    uint8_t received_char;
    
    // Initialize LED
    LED_init();
    
    // Initialize USART0 with interrupts
    USART0_init();
    
    // Enable global interrupts - CRITICAL for interrupt-driven operation
    sei();
    
    // Small delay to ensure USART is stable
    _delay_ms(100);
    
    // Display welcome message
    display_welcome();
    
    // Main loop
    while (1)
    {
        // Check if data is available (non-blocking)
        if (USART0_read(&received_char)) {
            // Data received - process it
            handle_received_char(received_char);
        }
        
        // Perform other tasks while waiting for data
        // This is the key advantage: CPU is free to do other work!
        background_tasks();
        
        // In a real application, you could:
        // - Read sensors
        // - Update displays
        // - Process algorithms
        // - Respond to other inputs
        // All while serial communication happens in the background
    }
    
    return 0;  // Never reached
}