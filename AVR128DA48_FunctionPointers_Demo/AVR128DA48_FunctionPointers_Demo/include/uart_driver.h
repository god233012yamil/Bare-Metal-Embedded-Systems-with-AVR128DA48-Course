#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stddef.h>

typedef void (*uart_rx_callback_t)(uint8_t data, void *context);

void uart0_init(uint32_t baud_rate);
void uart0_register_rx_callback(uart_rx_callback_t callback, void *context);
void uart0_write_byte(uint8_t data);
void uart0_write_string(const char *text);

#endif
