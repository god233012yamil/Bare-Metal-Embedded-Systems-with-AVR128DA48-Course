#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <stdbool.h>
#include "config.h"
#include "reset_manager.h"
#include "system_time.h"
#include "watchdog_manager.h"
#include "fault.h"
#include "app_config.h"
#include "uart.h"
#include "sensor.h"
#include "control.h"
#include "diagnostics.h"
#include "system_state.h"

static void clock_init(void);
static void gpio_init(void);
static void communication_task(void);
static void fault_button_task(void);

/**
 * @brief Configures the AVR128DA48 main clock to 24 MHz internal oscillator.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void clock_init(void)
{
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCHF_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0x00);
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
}

/**
 * @brief Configures board GPIO pins to safe startup states.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void gpio_init(void)
{
    LED_HEARTBEAT_PORT.DIR |= LED_HEARTBEAT_PIN;
    LED_HEARTBEAT_PORT.OUT &= (uint8_t)~LED_HEARTBEAT_PIN;

    FAULT_BUTTON_PORT.DIR &= (uint8_t)~FAULT_BUTTON_PIN;
    FAULT_BUTTON_PINCTRL = PORT_PULLUPEN_bm;
}

/**
 * @brief Simulates a communication task that reports progress to the watchdog manager.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void communication_task(void)
{
    static uint32_t last_comm_ms = 0;
    uint32_t now_ms = system_time_get_ms();

    if ((now_ms - last_comm_ms) >= COMM_PERIOD_MS)
    {
        last_comm_ms = now_ms;
        watchdog_manager_report_alive(HEALTH_COMM_TASK);
    }
}

/**
 * @brief Debounces the fault injection button and requests a controlled fault.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void fault_button_task(void)
{
    static bool previous_pressed = false;
    static uint32_t last_edge_ms = 0;
    bool pressed = ((FAULT_BUTTON_PORT.IN & FAULT_BUTTON_PIN) == 0U);
    uint32_t now_ms = system_time_get_ms();

    if (pressed != previous_pressed)
    {
        last_edge_ms = now_ms;
        previous_pressed = pressed;
    }

    if (pressed && system_time_elapsed(last_edge_ms, 30U))
    {
        system_state_force_fault();
    }
}

/**
 * @brief Application entry point for the reliability demo firmware.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     This function never returns.
 */
int main(void)
{
    uint8_t reset_reason;

    cli();
    reset_reason = reset_manager_capture_reason();
    clock_init();
    gpio_init();
    system_time_init();
    watchdog_manager_init();
    sei();

    fault_init(reset_reason);
    app_config_load_or_default();
    uart_init(app_config_get()->uart_baud_rate);
    sensor_init();
    control_init();
    diagnostics_init();
    system_state_init();

    diagnostics_print_startup_report();
    diagnostics_print_fault_report();

    while (1)
    {
        system_state_task();
        sensor_task();
        control_task();
        communication_task();
        diagnostics_task();
        fault_button_task();
        watchdog_manager_service_if_healthy();
    }
}