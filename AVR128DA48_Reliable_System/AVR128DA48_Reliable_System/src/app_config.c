#include <avr/eeprom.h>
#include "app_config.h"
#include "config.h"
#include "fault.h"

#define CONFIG_MAGIC             0xC0A55048UL
#define CONFIG_VERSION           1U
#define CONFIG_CRC_SEED          0xFFFFU

static app_config_t g_config;
static app_config_t EEMEM g_eeprom_config;

static uint16_t app_config_crc16(const uint8_t *data, uint16_t length);
static void app_config_set_defaults(app_config_t *config);

/**
 * @brief Loads application configuration and falls back to defaults if invalid.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void app_config_load_or_default(void)
{
    eeprom_read_block(&g_config, &g_eeprom_config, sizeof(g_config));

    if (!app_config_is_valid(&g_config))
    {
        fault_report(FAULT_CONFIG_INVALID);
        app_config_set_defaults(&g_config);
        eeprom_update_block(&g_config, &g_eeprom_config, sizeof(g_config));
    }
}

/**
 * @brief Returns the active application configuration.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Pointer to the active configuration.
 */
const app_config_t *app_config_get(void)
{
    return &g_config;
}

/**
 * @brief Validates the application configuration structure.
 *
 * Args:
 *     config: Pointer to the configuration to validate.
 *
 * Returns:
 *     true if the configuration is valid, otherwise false.
 */
bool app_config_is_valid(const app_config_t *config)
{
    uint16_t crc;

    if (config == 0)
    {
        return false;
    }

    if (config->magic != CONFIG_MAGIC)
    {
        return false;
    }

    if (config->version != CONFIG_VERSION)
    {
        return false;
    }

    if (config->length != sizeof(app_config_t))
    {
        return false;
    }

    if (config->sample_period_ms < 10U)
    {
        return false;
    }

    if (config->heartbeat_period_ms < 100U)
    {
        return false;
    }

    if (config->uart_baud_rate > 1000000UL)
    {
        return false;
    }

    crc = app_config_crc16((const uint8_t *)config, sizeof(app_config_t) - sizeof(config->crc));

    return (crc == config->crc);
}

/**
 * @brief Writes default application configuration values.
 *
 * Args:
 *     config: Pointer to the configuration structure to fill.
 *
 * Returns:
 *     None.
 */
static void app_config_set_defaults(app_config_t *config)
{
    if (config == 0)
    {
        return;
    }

    config->magic = CONFIG_MAGIC;
    config->version = CONFIG_VERSION;
    config->length = sizeof(app_config_t);
    config->sample_period_ms = SENSOR_PERIOD_MS;
    config->heartbeat_period_ms = HEARTBEAT_PERIOD_MS;
    config->uart_baud_rate = UART_BAUD_RATE;
    config->crc = app_config_crc16((const uint8_t *)config, sizeof(app_config_t) - sizeof(config->crc));
}

/**
 * @brief Calculates a CRC-16/CCITT-FALSE checksum.
 *
 * Args:
 *     data: Pointer to the byte buffer.
 *     length: Number of bytes to process.
 *
 * Returns:
 *     Calculated CRC value.
 */
static uint16_t app_config_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = CONFIG_CRC_SEED;

    if (data == 0)
    {
        return 0;
    }

    while (length > 0U)
    {
        crc ^= (uint16_t)(*data) << 8;

        for (uint8_t bit = 0; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc <<= 1;
            }
        }

        data++;
        length--;
    }

    return crc;
}
