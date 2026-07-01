#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "status.h"

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t length;
    uint16_t sample_period_ms;
    uint16_t heartbeat_period_ms;
    uint32_t uart_baud_rate;
    uint16_t crc;
} app_config_t;

void app_config_load_or_default(void);
const app_config_t *app_config_get(void);
bool app_config_is_valid(const app_config_t *config);

#endif
