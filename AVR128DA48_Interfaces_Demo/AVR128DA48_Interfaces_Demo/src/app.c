#include "app.h"

#include <stdint.h>
#include "bsp.h"
#include "protocol.h"

/**
 * Runs the interface demonstration application.
 *
 * Args:
 *   status_led: LED interface used as a hardware-independent status output.
 *   comm: Communication interface used by the protocol layer.
 */
void app_run(led_interface_t *status_led, const comm_interface_t *comm)
{
    const uint8_t message[] = {
        'I', 'F', 'A', 'C', 'E'
    };

    if ((status_led == NULL) || (status_led->toggle == NULL)) {
        return;
    }

    while (1) {
        status_led->toggle(status_led->context);
        (void)protocol_send_packet(comm, message, sizeof(message));
        bsp_delay_ms(500U);
    }
}
