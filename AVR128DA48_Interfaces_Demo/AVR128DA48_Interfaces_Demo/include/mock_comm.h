#ifndef MOCK_COMM_H
#define MOCK_COMM_H

#include <stddef.h>
#include <stdint.h>
#include "interfaces.h"

#define MOCK_COMM_BUFFER_SIZE 128U

typedef struct {
    uint8_t tx_buffer[MOCK_COMM_BUFFER_SIZE];
    size_t tx_index;
} mock_comm_context_t;

void mock_comm_create(comm_interface_t *interface, mock_comm_context_t *context);

#endif
