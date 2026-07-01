#include <avr/io.h>
#include "config.h"
#include "control.h"
#include "sensor.h"
#include "fault.h"
#include "system_time.h"
#include "watchdog_manager.h"

static uint8_t g_output_level = 0;
static uint32_t g_last_control_ms = 0;

static uint8_t control_calculate_output(int16_t temperature_c_x10);
static uint8_t control_clamp_output(uint8_t level);

/**
 * @brief Initializes the output pins used by the control module.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void control_init(void)
{
    LED_HEARTBEAT_PORT.DIR |= LED_HEARTBEAT_PIN;
    outputs_set_safe_state();
}

/**
 * @brief Runs the non-blocking control task.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void control_task(void)
{
    const sensor_data_t *sensor = sensor_get_latest();
    uint32_t now_ms = system_time_get_ms();

    if ((now_ms - g_last_control_ms) < CONTROL_PERIOD_MS)
    {
        watchdog_manager_report_alive(HEALTH_CONTROL_TASK);
        return;
    }

    g_last_control_ms = now_ms;

    if ((sensor == 0) || (!sensor->valid))
    {
        fault_report(FAULT_INVALID_TEMPERATURE);
        outputs_set_safe_state();
        return;
    }

    g_output_level = control_calculate_output(sensor->temperature_c_x10);
    g_output_level = control_clamp_output(g_output_level);

    watchdog_manager_report_alive(HEALTH_CONTROL_TASK);
}

/**
 * @brief Places all controlled outputs in a known safe state.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void outputs_set_safe_state(void)
{
    g_output_level = 0;
    LED_HEARTBEAT_PORT.OUT &= (uint8_t)~LED_HEARTBEAT_PIN;
}

/**
 * @brief Returns the current logical output level.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Output level from 0 to 100.
 */
uint8_t control_get_output_level(void)
{
    return g_output_level;
}

/**
 * @brief Calculates a bounded output level from temperature.
 *
 * Args:
 *     temperature_c_x10: Temperature in degrees C multiplied by 10.
 *
 * Returns:
 *     Logical output level from 0 to 100.
 */
static uint8_t control_calculate_output(int16_t temperature_c_x10)
{
    if (temperature_c_x10 <= 250)
    {
        return 0;
    }

    if (temperature_c_x10 >= 350)
    {
        return 100;
    }

    return (uint8_t)((temperature_c_x10 - 250) / 1);
}

/**
 * @brief Clamps an output command to the accepted actuator range.
 *
 * Args:
 *     level: Requested output level.
 *
 * Returns:
 *     Saturated output level.
 */
static uint8_t control_clamp_output(uint8_t level)
{
    if (level > 100U)
    {
        return 100U;
    }

    return level;
}
