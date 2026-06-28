#include "app_storage.h"

#include "storage_eeprom.h"
#include "utfs.h"

#include <stdio.h>
#include <string.h>

static app_config_t app_config;
static app_counter_t app_counter;
static utfs_file_t config_file;
static utfs_file_t counter_file;

static void load_default_config(void);
static void load_default_counter(void);

/**
 * Registers application objects with UTFS.
 *
 * The demo stores two named files in EEPROM:
 * config: General device configuration.
 * counter: A persistent counter incremented from the UART command line.
 */
void app_storage_init(void)
{
    storage_eeprom_init();
    utfs_init(false);
    utfs_baseaddress_set(STORAGE_EEPROM_BASE_ADDRESS);

    utfs_set(&config_file, "config", &app_config, sizeof(app_config));
    utfs_register(&config_file, UTFS_NOFLAGS, UTFS_NOOPT);

    utfs_set(&counter_file, "counter", &app_counter, sizeof(app_counter));
    utfs_register(&counter_file, UTFS_NOFLAGS, UTFS_NOOPT);
}

/**
 * Loads UTFS files from EEPROM or creates default data on first boot.
 */
void app_storage_load_or_defaults(void)
{
    utfs_result_e result = utfs_load();

    if (result != RES_OK) {
        load_default_config();
        load_default_counter();
        utfs_file_signature_set(&config_file, APP_CONFIG_SIGNATURE);
        utfs_file_signature_set(&counter_file, APP_COUNTER_SIGNATURE);
        app_storage_save();
        return;
    }

    if (utfs_file_signature(&config_file) != APP_CONFIG_SIGNATURE) {
        load_default_config();
        utfs_file_signature_set(&config_file, APP_CONFIG_SIGNATURE);
    }

    if (utfs_file_signature(&counter_file) != APP_COUNTER_SIGNATURE) {
        load_default_counter();
        utfs_file_signature_set(&counter_file, APP_COUNTER_SIGNATURE);
    }

    app_config.boot_count++;
    app_storage_save();
}

/**
 * Saves all registered UTFS files to EEPROM.
 */
void app_storage_save(void)
{
    (void)utfs_save();
}

/**
 * Prints the current application storage state.
 */
void app_storage_print(void)
{
    printf("\r\nUTFS AVR128DA48 demo\r\n");
    printf("Device name: %s\r\n", app_config.device_name);
    printf("Boot count: %lu\r\n", (unsigned long)app_config.boot_count);
    printf("Baudrate: %lu\r\n", (unsigned long)app_config.baudrate);
    printf("Sample period: %u ms\r\n", app_config.sample_period_ms);
    printf("Debug enabled: %u\r\n", app_config.debug_enabled);
    printf("Counter value: %lu\r\n", (unsigned long)app_counter.value);
}

/**
 * Increments the persistent counter and saves it to EEPROM.
 */
void app_storage_increment_counter(void)
{
    app_counter.value++;
    (void)utfs_save_file(&counter_file);
}

/**
 * Updates the debug flag and saves the configuration file.
 *
 * Args:
 *     enabled: Non-zero enables debug mode. Zero disables it.
 */
void app_storage_set_debug(uint8_t enabled)
{
    app_config.debug_enabled = (enabled != 0u) ? 1u : 0u;
    (void)utfs_save_file(&config_file);
}

/**
 * Updates the sample period and saves the configuration file.
 *
 * Args:
 *     period_ms: New period in milliseconds.
 */
void app_storage_set_sample_period(uint16_t period_ms)
{
    app_config.sample_period_ms = period_ms;
    (void)utfs_save_file(&config_file);
}

/**
 * Gets the RAM copy of the configuration file.
 *
 * Returns:
 *     Pointer to the active configuration object.
 */
app_config_t *app_storage_config_get(void)
{
    return &app_config;
}

/**
 * Gets the RAM copy of the counter file.
 *
 * Returns:
 *     Pointer to the active counter object.
 */
app_counter_t *app_storage_counter_get(void)
{
    return &app_counter;
}

static void load_default_config(void)
{
    memset(&app_config, 0, sizeof(app_config));
    app_config.boot_count = 0u;
    app_config.baudrate = 115200u;
    app_config.sample_period_ms = 1000u;
    app_config.debug_enabled = 1u;
    strcpy(app_config.device_name, "AVR128DA48");
}

static void load_default_counter(void)
{
    memset(&app_counter, 0, sizeof(app_counter));
}
