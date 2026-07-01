#ifndef UART_COMM_H
#define UART_COMM_H

#include <avr/io.h>
#include <stdint.h>
#include "interfaces.h"

typedef struct {
    USART_t *usart;
} uart_comm_context_t;

void uart_comm_create(comm_interface_t *interface,
                      uart_comm_context_t *context,
                      USART_t *usart,
                      uint32_t baudrate);

#endif
