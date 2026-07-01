#ifndef FAULT_H_
#define FAULT_H_

#include <stdint.h>
#include <stdbool.h>
#include "status.h"

typedef enum
{
    FAULT_NONE = 0,
    FAULT_SENSOR_READ_FAILED,
    FAULT_INVALID_TEMPERATURE,
    FAULT_UART_TX_TIMEOUT,
    FAULT_INVALID_STATE,
    FAULT_STACK_LOW,
    FAULT_CONFIG_INVALID,
    FAULT_WATCHDOG_RESET,
    FAULT_FORCED_BY_BUTTON,
    FAULT_RECOVERY_FAILED
} fault_code_t;

typedef struct
{
    uint32_t magic;
    uint32_t boot_count;
    uint32_t failed_boot_count;
    uint8_t reset_reason;
    uint8_t fault_code;
    uint8_t system_state;
    uint8_t reserved;
    uint32_t uptime_ms;
    uint16_t crc;
} fault_record_t;

void fault_init(uint8_t reset_reason);
void fault_report(fault_code_t fault);
fault_code_t fault_get_last(void);
uint32_t fault_get_boot_count(void);
uint32_t fault_get_failed_boot_count(void);
void fault_mark_boot_successful(void);
void fault_save_snapshot(uint8_t system_state);
bool fault_safe_mode_required(void);
const fault_record_t *fault_get_record(void);

#endif
