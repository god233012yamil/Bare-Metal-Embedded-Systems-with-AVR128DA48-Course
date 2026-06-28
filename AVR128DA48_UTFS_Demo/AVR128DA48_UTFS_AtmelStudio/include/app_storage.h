#ifndef APP_STORAGE_H_
#define APP_STORAGE_H_

#include <stdint.h>

#define APP_CONFIG_SIGNATURE 0xA128u
#define APP_COUNTER_SIGNATURE 0xC048u

typedef struct {
    uint32_t boot_count;
    uint32_t baudrate;
    uint16_t sample_period_ms;
    uint8_t debug_enabled;
    char device_name[16];
} app_config_t;

typedef struct {
    uint32_t value;
} app_counter_t;

void app_storage_init(void);
void app_storage_load_or_defaults(void);
void app_storage_save(void);
void app_storage_print(void);
void app_storage_increment_counter(void);
void app_storage_set_debug(uint8_t enabled);
void app_storage_set_sample_period(uint16_t period_ms);
app_config_t *app_storage_config_get(void);
app_counter_t *app_storage_counter_get(void);

#endif
