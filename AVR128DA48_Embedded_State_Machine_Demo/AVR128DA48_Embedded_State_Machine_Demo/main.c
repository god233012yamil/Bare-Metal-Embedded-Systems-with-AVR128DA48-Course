#include "app.h"
#include "button.h"
#include "hardware.h"
#include "system_time.h"
#include "uart.h"

#include <avr/interrupt.h>
#include <stdint.h>

/**
 * Initializes the platform and runs the cooperative event-processing loop.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     This function never returns during normal operation.
 */
int main(void)
{
    uint32_t now_ms;

    hardware_init();
    uart_init();
    system_time_init();

    now_ms = system_time_get_ms();
    button_init(now_ms);
    app_init(now_ms);

    // Enable interrupts only after all interrupt-driven peripherals are ready.
    sei();

    while (1)
    {
        now_ms = system_time_get_ms();

        if (button_update(now_ms))
        {
            app_post_event(APP_EVENT_BUTTON_PRESSED, now_ms, 0U);
        }

        app_update(now_ms);
        app_process_events();
    }
}