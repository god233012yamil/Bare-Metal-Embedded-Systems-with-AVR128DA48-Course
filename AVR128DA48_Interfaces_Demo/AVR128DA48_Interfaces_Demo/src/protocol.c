#include "protocol.h"

#define PROTOCOL_START_BYTE 0xAAU

/**
 * Sends a simple packet through any communication interface.
 *
 * Args:
 *   comm: Communication interface used to transmit the packet.
 *   payload: Pointer to payload bytes.
 *   payload_length: Number of payload bytes.
 *
 * Returns:
 *   0 on success, otherwise a negative error code.
 */
int protocol_send_packet(const comm_interface_t *comm,
                         const uint8_t *payload,
                         size_t payload_length)
{
    uint8_t header[2];

    if ((comm == NULL) || (comm->write == NULL) || (payload == NULL)) {
        return -1;
    }

    if (payload_length > 255U) {
        return -2;
    }

    header[0] = PROTOCOL_START_BYTE;
    header[1] = (uint8_t)payload_length;

    if (comm->write(comm->context, header, sizeof(header)) != (int)sizeof(header)) {
        return -3;
    }

    if (comm->write(comm->context, payload, payload_length) != (int)payload_length) {
        return -4;
    }

    return 0;
}
