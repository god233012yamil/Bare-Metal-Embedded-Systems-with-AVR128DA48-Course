#include <avr/io.h>
#include "config.h"
#include "sensor.h"
#include "fault.h"
#include "system_time.h"
#include "watchdog_manager.h"

static sensor_data_t g_latest_sensor_data;
static uint32_t g_last_sensor_read_ms = 0;

static bool sensor_temperature_is_valid(int16_t temperature_c_x10);

/**
 * @brief Initializes the demo sensor module.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void sensor_init(void)
{
    g_latest_sensor_data.temperature_c_x10 = 250;
    g_latest_sensor_data.valid = true;
}

/**
 * @brief Runs the non-blocking sensor task.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void sensor_task(void)
{
    sensor_data_t data;
    uint32_t now_ms = system_time_get_ms();

    if ((now_ms - g_last_sensor_read_ms) < SENSOR_PERIOD_MS)
    {
        watchdog_manager_report_alive(HEALTH_SENSOR_TASK);
        return;
    }

    g_last_sensor_read_ms = now_ms;

    if (sensor_read(&data) == STATUS_OK)
    {
        g_latest_sensor_data = data;
        watchdog_manager_report_alive(HEALTH_SENSOR_TASK);
    }
    else
    {
        g_latest_sensor_data.valid = false;
        fault_report(FAULT_SENSOR_READ_FAILED);
    }
}

/**
 * @brief Reads the simulated temperature sensor used by the reliability demo.
 *
 * Args:
 *     data: Destination structure for the sensor sample.
 *
 * Returns:
 *     STATUS_OK if the sample is valid, otherwise an error code.
 */
status_t sensor_read(sensor_data_t *data)
{
    static int16_t simulated_temperature_c_x10 = 240;
    static int8_t direction = 1;

    if (data == 0)
    {
        return STATUS_INVALID_ARG;
    }

    simulated_temperature_c_x10 += direction;

    if (simulated_temperature_c_x10 >= 310)
    {
        direction = -1;
    }
    else if (simulated_temperature_c_x10 <= 220)
    {
        direction = 1;
    }

    if (!sensor_temperature_is_valid(simulated_temperature_c_x10))
    {
        data->valid = false;
        return STATUS_HW_ERROR;
    }

    data->temperature_c_x10 = simulated_temperature_c_x10;
    data->valid = true;

    return STATUS_OK;
}

/**
 * @brief Returns the latest valid sensor sample.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Pointer to the latest sensor data structure.
 */
const sensor_data_t *sensor_get_latest(void)
{
    return &g_latest_sensor_data;
}

/**
 * @brief Validates the simulated temperature value.
 *
 * Args:
 *     temperature_c_x10: Temperature in degrees C multiplied by 10.
 *
 * Returns:
 *     true if the sample is inside the accepted range, otherwise false.
 */
static bool sensor_temperature_is_valid(int16_t temperature_c_x10)
{
    if (temperature_c_x10 < -400)
    {
        return false;
    }

    if (temperature_c_x10 > 1250)
    {
        return false;
    }

    return true;
}
