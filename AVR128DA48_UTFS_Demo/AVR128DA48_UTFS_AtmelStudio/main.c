#include <avr/io.h>

#include "app_storage.h"
#include "clkctrl.h"
#include "storage_eeprom.h"
#include "uart0.h"
#include "utfs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYSTEM_CLOCK_HZ 12000000UL
#define UART_BAUDRATE 115200u
#define COMMAND_BUFFER_SIZE 32u

static int uart_putchar_printf(char c, FILE *stream);
static void print_help(void);
static void process_command(char *command);
static void poll_command_line(void);

static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar_printf, NULL, _FDEV_SETUP_WRITE);
static char command_buffer[COMMAND_BUFFER_SIZE];
static uint8_t command_index;

static int uart_putchar_printf(char c, FILE *stream)
{
    (void)stream;

    if (c == '\n') {
        uart0_write_char('\r');
    }

    uart0_write_char(c);
    return 0;
}

/**
 * Prints the serial command menu.
 */
static void print_help(void)
{
    printf("\r\nCommands:\r\n");
    printf("  help           Show this menu\r\n");
    printf("  show           Print stored UTFS data\r\n");
    printf("  inc            Increment and save the counter file\r\n");
    printf("  debug 0        Disable debug flag\r\n");
    printf("  debug 1        Enable debug flag\r\n");
    printf("  period <ms>    Set sample period\r\n");
    printf("  save           Save all UTFS files\r\n");
    printf("  erase          Erase EEPROM region and reload defaults\r\n");
    printf("  status         Print registered UTFS entries\r\n");
    printf("\r\n> ");
}

/**
 * Polls USART0 and assembles a CR/LF terminated command line.
 */
static void poll_command_line(void)
{
    char c;

    if (!uart0_read_char_nonblocking(&c)) {
        return;
    }

    if ((c == '\r') || (c == '\n')) {
        command_buffer[command_index] = '\0';
        printf("\r\n");

        if (command_index > 0u) {
            process_command(command_buffer);
        }

        command_index = 0u;
        printf("> ");
        return;
    }

    if ((c == '\b') || (c == 0x7Fu)) {
        if (command_index > 0u) {
            command_index--;
            printf("\b \b");
        }
        return;
    }

    if (command_index < (COMMAND_BUFFER_SIZE - 1u)) {
        command_buffer[command_index++] = c;
        uart0_write_char(c);
    }
}

/**
 * Executes one serial command.
 *
 * Args:
 *     command: Null-terminated command string from the serial interface.
 */
static void process_command(char *command)
{
    if (strcmp(command, "help") == 0) {
        print_help();
    } else if (strcmp(command, "show") == 0) {
        app_storage_print();
    } else if (strcmp(command, "inc") == 0) {
        app_storage_increment_counter();
        printf("Counter saved: %lu\r\n", (unsigned long)app_storage_counter_get()->value);
    } else if (strncmp(command, "debug ", 6u) == 0) {
        uint8_t enabled = (uint8_t)atoi(&command[6]);
        app_storage_set_debug(enabled);
        printf("Debug flag saved: %u\r\n", app_storage_config_get()->debug_enabled);
    } else if (strncmp(command, "period ", 7u) == 0) {
        uint16_t period_ms = (uint16_t)atoi(&command[7]);
        app_storage_set_sample_period(period_ms);
        printf("Sample period saved: %u ms\r\n", app_storage_config_get()->sample_period_ms);
    } else if (strcmp(command, "save") == 0) {
        app_storage_save();
        printf("All files saved.\r\n");
    } else if (strcmp(command, "erase") == 0) {
        storage_eeprom_erase_region();
        app_storage_load_or_defaults();
        printf("EEPROM region erased and defaults restored.\r\n");
    } else if (strcmp(command, "status") == 0) {
        utfs_status();
    } else {
        printf("Unknown command. Type help.\r\n");
    }
}

/**
 * Application entry point.
 *
 * The demo initializes USART0, registers two UTFS files, loads them from EEPROM,
 * and exposes a small command interface over the serial port.
 *
 * Returns:
 *     This function never returns.
 */
int main(void)
{
    // Initialize the clock
	if (!CLKCTRL_init(SYSTEM_CLOCK_HZ)) {
        while (true) {
            /* Invalid clock configuration. */
        }
    }
	
	// Initialize the UART0
    if (!uart0_init(SYSTEM_CLOCK_HZ, UART_BAUDRATE)) {
        while (true) {
            /* Requested baud cannot be generated from CLK_PER. */
        }
    }
	
    stdout = &uart_stdout;

    printf("\r\nAVR128DA48 UTFS EEPROM demo\r\n");

    app_storage_init();
    app_storage_load_or_defaults();
    app_storage_print();
    print_help();

    while (true) {
        poll_command_line();
    }
}
