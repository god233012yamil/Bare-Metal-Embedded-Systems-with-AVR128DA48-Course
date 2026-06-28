#ifndef UART0_H_
#define UART0_H_

#include <stdbool.h>
#include <stdint.h>

bool uart0_init(uint32_t peripheral_clock_hz, uint32_t baudrate);
void uart0_write_char(char c);
void uart0_write_string(const char *text);
bool uart0_read_char_nonblocking(char *c);

#endif
