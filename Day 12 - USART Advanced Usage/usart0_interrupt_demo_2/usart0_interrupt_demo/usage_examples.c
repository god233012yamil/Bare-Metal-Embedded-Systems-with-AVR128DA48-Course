/*
 * usage_examples.c
 *
 * Practical Examples of Using Interrupt-Driven USART
 * 
 * This file demonstrates various common use cases and patterns
 * for the interrupt-driven USART driver with ring buffers.
 *
 * Copy these examples into your main.c as needed.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart0_interrupt.h"

// ============================================================================
// EXAMPLE 1: Basic Echo (Simplest Use Case)
// ============================================================================
/**
 * @brief Simple echo example - echoes back received characters
 * 
 * Demonstrates:
 * - Non-blocking read
 * - Non-blocking write
 * - Minimal code for basic communication
 */
void example_simple_echo(void)
{
    uint8_t data;
    
    USART0_init();
	
    sei();  // Enable global interrupts
    
    while (1) {
        // Check if data received
        if (USART0_read(&data)) {
            // Echo it back
            USART0_write(data);
            
            // Add line feed after carriage return
            if (data == '\r') {
                USART0_write('\n');
            }
        }
        
        // CPU is free here to do other work!
    }
}

// ============================================================================
// EXAMPLE 2: Line-Based Input (Command Processing)
// ============================================================================
/**
 * @brief Receive complete lines of text
 * 
 * Demonstrates:
 * - Building up a string from individual characters
 * - Line-based processing
 * - Buffer overflow protection
 */
void example_line_input(void)
{
    char line_buffer[64];
    uint8_t index = 0;
    uint8_t data;
    
    USART0_init();
	
    sei();
    
    USART0_writeString_blocking("Enter text (press Enter):\r\n");
    
    while (1) {
        if (USART0_read(&data)) {
            // Echo character
            USART0_write(data);
            
            // Check for line ending
            if (data == '\r' || data == '\n') {
                // Null-terminate string
                line_buffer[index] = '\0';
                
                // Process the line
                USART0_writeString_blocking("\r\nYou entered: ");
                USART0_writeString_blocking(line_buffer);
                USART0_writeString_blocking("\r\n");
                
                // Reset for next line
                index = 0;
            }
            // Handle backspace
            else if (data == '\b' || data == 127) {
                if (index > 0) {
                    index--;
                    USART0_writeString_blocking(" \b");  // Erase on screen
                }
            }
            // Regular character
            else if (index < sizeof(line_buffer) - 1) {
                line_buffer[index++] = data;
            }
        }
    }
}

// ============================================================================
// EXAMPLE 3: Sending Sensor Data Periodically
// ============================================================================
/**
 * @brief Transmit sensor readings at regular intervals
 * 
 * Demonstrates:
 * - Background transmission
 * - Using printf for formatted output
 * - CPU remains free to sample sensors
 */
void example_sensor_telemetry(void)
{
    uint16_t adc_value;
    uint32_t timestamp = 0;
    
    USART0_init();
	
    sei();
    
    while (1) {
        // Simulate reading sensor (replace with actual ADC read)
        adc_value = 512 + (timestamp % 100);
        
        // Send formatted data - non-blocking!
        USART0_printf("Time: %lu, ADC: %u, Voltage: %u.%02u V\r\n",
                      timestamp,
                      adc_value,
                      (adc_value * 5) / 1024,
                      ((adc_value * 500) / 1024) % 100);
        
        // Wait 1 second (in real app, use timer interrupt)
        _delay_ms(1000);
        timestamp++;
        
        // CPU could be doing other tasks during delay
    }
}

// ============================================================================
// EXAMPLE 4: Binary Protocol Communication
// ============================================================================
/**
 * @brief Implement a simple binary protocol
 * 
 * Protocol format: [START] [LENGTH] [DATA...] [CHECKSUM]
 * - START: 0xAA
 * - LENGTH: Number of data bytes
 * - DATA: Payload
 * - CHECKSUM: XOR of all bytes
 * 
 * Demonstrates:
 * - State machine for packet reception
 * - Binary data handling
 * - Checksum verification
 */

typedef enum {
    STATE_WAIT_START,
    STATE_WAIT_LENGTH,
    STATE_WAIT_DATA,
    STATE_WAIT_CHECKSUM
} protocol_state_t;

void example_binary_protocol(void)
{
    protocol_state_t state = STATE_WAIT_START;
    uint8_t packet_data[64];
    uint8_t packet_length = 0;
    uint8_t packet_index = 0;
    uint8_t checksum = 0;
    uint8_t data;
    
    USART0_init();
    sei();
    
    while (1) {
        if (USART0_read(&data)) {
            switch (state) {
                case STATE_WAIT_START:
                    if (data == 0xAA) {
                        checksum = data;
                        state = STATE_WAIT_LENGTH;
                    }
                    break;
                    
                case STATE_WAIT_LENGTH:
                    packet_length = data;
                    checksum ^= data;
                    packet_index = 0;
                    state = STATE_WAIT_DATA;
                    break;
                    
                case STATE_WAIT_DATA:
                    packet_data[packet_index++] = data;
                    checksum ^= data;
                    
                    if (packet_index >= packet_length) {
                        state = STATE_WAIT_CHECKSUM;
                    }
                    break;
                    
                case STATE_WAIT_CHECKSUM:
                    if (data == checksum) {
                        // Valid packet - process it
                        USART0_writeString_blocking("Valid packet received\r\n");
                        
                        // Send ACK
                        USART0_write(0xAA);  // START
                        USART0_write(0x01);  // LENGTH = 1
                        USART0_write(0x06);  // DATA = ACK
                        USART0_write(0xAA ^ 0x01 ^ 0x06);  // CHECKSUM
                    } else {
                        // Checksum error
                        USART0_writeString_blocking("Checksum error\r\n");
                        
                        // Send NAK
                        USART0_write(0xAA);  // START
                        USART0_write(0x01);  // LENGTH = 1
                        USART0_write(0x15);  // DATA = NAK
                        USART0_write(0xAA ^ 0x01 ^ 0x15);  // CHECKSUM
                    }
                    
                    state = STATE_WAIT_START;
                    break;
            }
        }
    }
}

// ============================================================================
// EXAMPLE 5: Buffered Bulk Data Transfer
// ============================================================================
/**
 * @brief Send large amounts of data efficiently
 * 
 * Demonstrates:
 * - Checking buffer space before writing
 * - Handling buffer full conditions
 * - Efficient bulk transfers
 */
void example_bulk_transfer(void)
{
    uint8_t large_data[512];
    uint16_t index = 0;
    uint16_t total_sent = 0;
    
    USART0_init();
	
    sei();
    
    // Fill array with data (example: 0-255 pattern)
    for (uint16_t i = 0; i < sizeof(large_data); i++) {
        large_data[i] = i & 0xFF;
    }
    
    USART0_writeString_blocking("Starting bulk transfer...\r\n");
    
    while (total_sent < sizeof(large_data)) {
        // Check if there's space in TX buffer
        if (USART0_txFreeSpace() > 0) {
            // Send next byte
            if (USART0_write(large_data[index])) {
                index++;
                total_sent++;
            }
        }
        
        // Could do other work here while waiting for buffer space
        // The interrupt handler is transmitting in the background
    }
    
    // Wait for all data to be transmitted
    while (!USART0_txComplete()) {
        ; // Wait
    }
    
    USART0_writeString_blocking("\r\nTransfer complete!\r\n");
}

// ============================================================================
// EXAMPLE 6: Menu System
// ============================================================================
/**
 * @brief Interactive menu with multiple options
 * 
 * Demonstrates:
 * - Menu display
 * - Single character commands
 * - Multiple action handlers
 */
void example_menu_system(void)
{
    uint8_t choice;
    uint16_t counter = 0;
    
    USART0_init();
    sei();
    
    // Display menu
    USART0_writeString_blocking("\r\n=== Main Menu ===\r\n");
    USART0_writeString_blocking("1. Display counter\r\n");
    USART0_writeString_blocking("2. Increment counter\r\n");
    USART0_writeString_blocking("3. Reset counter\r\n");
    USART0_writeString_blocking("4. Show status\r\n");
    USART0_writeString_blocking("Enter choice: ");
    
    while (1) {
        if (USART0_read(&choice)) {
            USART0_write(choice);  // Echo
            USART0_writeString_blocking("\r\n");
            
            switch (choice) {
                case '1':
                    USART0_printf("Counter value: %u\r\n", counter);
                    break;
                    
                case '2':
                    counter++;
                    USART0_printf("Counter incremented to %u\r\n", counter);
                    break;
                    
                case '3':
                    counter = 0;
                    USART0_writeString_blocking("Counter reset\r\n");
                    break;
                    
                case '4':
                    USART0_printf("RX available: %u bytes\r\n", USART0_available());
                    USART0_printf("TX free: %u bytes\r\n", USART0_txFreeSpace());
                    break;
                    
                default:
                    USART0_writeString_blocking("Invalid choice\r\n");
                    break;
            }
            
            USART0_writeString_blocking("\r\nEnter choice: ");
        }
    }
}

// ============================================================================
// EXAMPLE 7: Data Logging with Timestamps
// ============================================================================
/**
 * @brief Log events with timestamps
 * 
 * Demonstrates:
 * - Timestamped logging
 * - Multiple severity levels
 * - Formatted output
 */

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} log_level_t;

void log_message(log_level_t level, const char *message)
{
    static uint32_t timestamp = 0;
    const char *level_str;
    
    // Increment timestamp (in real app, use RTC or timer)
    timestamp++;
    
    // Convert level to string
    switch (level) {
        case LOG_INFO:    level_str = "INFO"; break;
        case LOG_WARNING: level_str = "WARN"; break;
        case LOG_ERROR:   level_str = "ERR "; break;
        default:          level_str = "????"; break;
    }
    
    // Send formatted log message
    USART0_printf("[%06lu] [%s] %s\r\n", timestamp, level_str, message);
}

void example_logging(void)
{
    USART0_init();
    sei();
    
    log_message(LOG_INFO, "System initialized");
    _delay_ms(100);
    
    log_message(LOG_INFO, "Starting main loop");
    _delay_ms(100);
    
    log_message(LOG_WARNING, "Temperature high");
    _delay_ms(100);
    
    log_message(LOG_ERROR, "Sensor failure");
    _delay_ms(100);
    
    while (1) {
        // Main application
    }
}

// ============================================================================
// EXAMPLE 8: AT-Command Style Interface (like ESP8266/GSM modules)
// ============================================================================
/**
 * @brief Implement AT command processing
 * 
 * Demonstrates:
 * - Command parsing
 * - Response formatting
 * - Multiple command types
 */
void example_at_commands(void)
{
    char cmd_buffer[32];
    uint8_t index = 0;
    uint8_t data;
    
    USART0_init();
	
    sei();
    
    USART0_writeString_blocking("AT Command Interface Ready\r\n");
    
    while (1) {
        if (USART0_read(&data)) {
            if (data == '\r' || data == '\n') {
                cmd_buffer[index] = '\0';
                
                // Process AT commands
                if (strcmp(cmd_buffer, "AT") == 0) {
                    USART0_writeString_blocking("OK\r\n");
                }
                else if (strcmp(cmd_buffer, "AT+VER?") == 0) {
                    USART0_writeString_blocking("+VER:1.0.0\r\n");
                    USART0_writeString_blocking("OK\r\n");
                }
                else if (strncmp(cmd_buffer, "AT+SEND=", 8) == 0) {
                    USART0_writeString_blocking("SENDING: ");
                    USART0_writeString_blocking(cmd_buffer + 8);
                    USART0_writeString_blocking("\r\nOK\r\n");
                }
                else if (strcmp(cmd_buffer, "AT+RESET") == 0) {
                    USART0_writeString_blocking("RESETTING...\r\n");
                    _delay_ms(100);
                    // Perform reset (watchdog or software reset)
                }
                else {
                    USART0_writeString_blocking("ERROR\r\n");
                }
                
                index = 0;
            }
            else if (index < sizeof(cmd_buffer) - 1) {
                cmd_buffer[index++] = data;
            }
        }
    }
}

// ============================================================================
// EXAMPLE 9: Handling Multiple Data Streams
// ============================================================================
/**
 * @brief Process different data types simultaneously
 * 
 * Demonstrates:
 * - Message framing
 * - Multiple data channels
 * - Parallel processing
 */
void example_multiple_streams(void)
{
    uint8_t data;
    
    USART0_init();
    sei();
    
    while (1) {
        if (USART0_read(&data)) {
            // Stream identifier: first byte indicates type
            if (data == 'D') {
                // Debug message stream
                USART0_writeString_blocking("[DEBUG] ");
                while (USART0_read(&data) && data != '\n') {
                    USART0_write(data);
                }
                USART0_writeString_blocking("\r\n");
            }
            else if (data == 'T') {
                // Telemetry data stream
                uint8_t sensor_id, value;
                if (USART0_read(&sensor_id) && USART0_read(&value)) {
                    USART0_printf("Sensor %u: %u\r\n", sensor_id, value);
                }
            }
            else if (data == 'C') {
                // Command stream
                uint8_t cmd;
                if (USART0_read(&cmd)) {
                    USART0_printf("Executing command: 0x%02X\r\n", cmd);
                    // Execute command...
                }
            }
        }
    }
}

// ============================================================================
// EXAMPLE 10: Non-Blocking Timeout Pattern
// ============================================================================
/**
 * @brief Wait for data with timeout
 * 
 * Demonstrates:
 * - Timeout handling
 * - Non-blocking wait
 * - Timer integration
 */
bool read_with_timeout(uint8_t *data, uint16_t timeout_ms)
{
    uint16_t elapsed = 0;
    
    while (elapsed < timeout_ms) {
        if (USART0_read(data)) {
            return true;  // Data received
        }
        
        _delay_ms(1);
        elapsed++;
    }
    
    return false;  // Timeout
}

void example_timeout(void)
{
    uint8_t data;
    
    USART0_init();
    sei();
    
    USART0_writeString_blocking("Waiting for data (5 second timeout)...\r\n");
    
    if (read_with_timeout(&data, 5000)) {
        USART0_printf("Received: %c\r\n", data);
    } else {
        USART0_writeString_blocking("Timeout!\r\n");
    }
    
    while (1) {
        // Continue...
    }
}
