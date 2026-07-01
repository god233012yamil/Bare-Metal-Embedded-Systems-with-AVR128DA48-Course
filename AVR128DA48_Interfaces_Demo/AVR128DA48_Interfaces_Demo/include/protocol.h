#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "interfaces.h"

int protocol_send_packet(const comm_interface_t *comm,
                         const uint8_t *payload,
                         size_t payload_length);

#endif
