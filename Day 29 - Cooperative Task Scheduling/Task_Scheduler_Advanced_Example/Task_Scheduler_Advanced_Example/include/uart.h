#ifndef UART_H
#define UART_H

#include <stdint.h>

/*
 * UART0 on PC0 (TX) / PC1 (RX) - default CDC virtual COM port on Curiosity Nano
 *
 * Baud = 115200 @ F_CPU = 4 MHz
 *   BAUD register (normal mode) = (64 * F_CPU) / (16 * baud)
 *                               = (64 * 4000000) / (16 * 115200)
 *                               = 138.88  -> 139
 */

#define UART_BAUD_REG   139u

void uart_init(void);
void uart_send_byte(uint8_t byte);
void uart_print_string(const char *str);
void uart_print_uint16(uint16_t val);
void uart_print_uint32(uint32_t val);

#endif /* UART_H */
