#include "mock_comm.h"

#include <string.h>

/**
 * Stores transmitted bytes in a RAM buffer for testing.
 *
 * Args:
 *   context: Pointer to a mock_comm_context_t instance.
 *   data: Pointer to bytes to store.
 *   length: Number of bytes to store.
 *
 * Returns:
 *   Number of bytes stored, or -1 if the mock buffer overflows.
 */
static int mock_write_impl(void *context, const uint8_t *data, size_t length)
{
    mock_comm_context_t *mock = (mock_comm_context_t *)context;

    if ((mock == NULL) || (data == NULL)) {
        return -1;
    }

    if ((mock->tx_index + length) > MOCK_COMM_BUFFER_SIZE) {
        return -1;
    }

    memcpy(&mock->tx_buffer[mock->tx_index], data, length);
    mock->tx_index += length;

    return (int)length;
}

/**
 * Reads bytes from the mock interface.
 *
 * Args:
 *   context: Pointer to a mock_comm_context_t instance.
 *   data: Destination buffer.
 *   length: Maximum number of bytes to read.
 *
 * Returns:
 *   Number of bytes read. The demo mock has no receive source, so it returns 0.
 */
static int mock_read_impl(void *context, uint8_t *data, size_t length)
{
    (void)context;
    (void)data;
    (void)length;

    return 0;
}

/**
 * Creates a mock implementation of the generic communication interface.
 *
 * Args:
 *   interface: Generic communication interface to initialize.
 *   context: Mock communication private state storage.
 */
void mock_comm_create(comm_interface_t *interface, mock_comm_context_t *context)
{
    memset(context->tx_buffer, 0, sizeof(context->tx_buffer));
    context->tx_index = 0U;

    interface->context = context;
    interface->write = mock_write_impl;
    interface->read = mock_read_impl;
}
