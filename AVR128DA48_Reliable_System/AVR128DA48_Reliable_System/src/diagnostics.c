#include <avr/io.h>
#include <stdint.h>
#include "config.h"
#include "diagnostics.h"
#include "fault.h"
#include "sensor.h"
#include "control.h"
#include "system_state.h"
#include "system_time.h"
#include "uart.h"
#include "watchdog_manager.h"

extern uint8_t __heap_start;
extern uint8_t __stack;

static uint32_t g_last_diagnostics_ms = 0;
static void diagnostics_print_crlf(void);

/**
 * @brief Initializes the diagnostics module.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void diagnostics_init(void)
{
    g_last_diagnostics_ms = 0;
}

/**
 * @brief Runs periodic diagnostics and reports task health.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void diagnostics_task(void)
{
    uint32_t now_ms = system_time_get_ms();

    if ((now_ms - g_last_diagnostics_ms) < DIAGNOSTICS_PERIOD_MS)
    {
        watchdog_manager_report_alive(HEALTH_DIAGNOSTICS_TASK);
        return;
    }

    g_last_diagnostics_ms = now_ms;

    if (diagnostics_get_stack_unused_bytes() < STACK_LOW_WATERMARK_BYTES)
    {
        fault_report(FAULT_STACK_LOW);
    }

    uart_write_string("uptime_ms=");
    uart_write_u32(now_ms);
    uart_write_string(", state=");
    uart_write_u32((uint32_t)system_state_get());
    uart_write_string(", temp_x10=");
    uart_write_u32((uint32_t)sensor_get_latest()->temperature_c_x10);
    uart_write_string(", output=");
    uart_write_u32((uint32_t)control_get_output_level());
    uart_write_string(", last_fault=");
    uart_write_u32((uint32_t)fault_get_last());
    diagnostics_print_crlf();

    watchdog_manager_report_alive(HEALTH_DIAGNOSTICS_TASK);
}

/**
 * @brief Prints the startup report over UART.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void diagnostics_print_startup_report(void)
{
    const fault_record_t *record = fault_get_record();

    uart_write_string("\r\nAVR128DA48 Reliable Embedded System Demo\r\n");
    uart_write_string("boot_count=");
    uart_write_u32(record->boot_count);
    uart_write_string(", failed_boot_count=");
    uart_write_u32(record->failed_boot_count);
    uart_write_string(", reset_reason=0x");
    uart_write_hex8(record->reset_reason);
    diagnostics_print_crlf();
}

/**
 * @brief Prints the most recent persistent fault report.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void diagnostics_print_fault_report(void)
{
    const fault_record_t *record = fault_get_record();

    uart_write_string("fault_code=");
    uart_write_u32(record->fault_code);
    uart_write_string(", fault_state=");
    uart_write_u32(record->system_state);
    uart_write_string(", fault_uptime_ms=");
    uart_write_u32(record->uptime_ms);
    diagnostics_print_crlf();
}

/**
 * @brief Estimates unused stack space using linker symbols.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Approximate free stack space in bytes.
 */
uint16_t diagnostics_get_stack_unused_bytes(void)
{
    uint8_t local_marker;
    uintptr_t current_stack = (uintptr_t)&local_marker;
    uintptr_t heap_start = (uintptr_t)&__heap_start;

    if (current_stack <= heap_start)
    {
        return 0;
    }

    return (uint16_t)(current_stack - heap_start);
}

/**
 * @brief Prints a carriage return and line feed sequence.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void diagnostics_print_crlf(void)
{
    uart_write_string("\r\n");
}
