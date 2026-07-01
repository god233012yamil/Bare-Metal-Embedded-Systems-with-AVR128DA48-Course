#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "status.h"

void uart_init(uint32_t baud_rate);
status_t uart_write_byte(uint8_t data, uint32_t timeout_ms);
status_t uart_write_string(const char *text);
status_t uart_write_u32(uint32_t value);
status_t uart_write_hex8(uint8_t value);

#endif
