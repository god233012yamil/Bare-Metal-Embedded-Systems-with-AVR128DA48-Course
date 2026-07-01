#include <avr/io.h>
#include "app.h"
#include "bsp.h"
#include "gpio_led.h"
#include "interfaces.h"
#include "uart_comm.h"

/**
 * Application entry point.
 *
 * Returns:
 *   This function never returns during normal operation.
 */
int main(void)
{
    led_interface_t status_led;
    gpio_led_context_t status_led_context;

    comm_interface_t console;
    uart_comm_context_t console_context;

    bsp_init();

    gpio_led_create(&status_led, &status_led_context, &PORTA, PIN0_bm);
    uart_comm_create(&console, &console_context, &USART0, 115200UL);

    app_run(&status_led, &console);

    return 0;
}
