#ifndef WATCHDOG_MANAGER_H_
#define WATCHDOG_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>

#define HEALTH_SENSOR_TASK         (1UL << 0)
#define HEALTH_CONTROL_TASK        (1UL << 1)
#define HEALTH_COMM_TASK           (1UL << 2)
#define HEALTH_DIAGNOSTICS_TASK    (1UL << 3)
#define HEALTH_SYSTEM_TASK         (1UL << 4)

#define HEALTH_ALL_TASKS           (HEALTH_SENSOR_TASK | HEALTH_CONTROL_TASK | HEALTH_COMM_TASK | HEALTH_DIAGNOSTICS_TASK | HEALTH_SYSTEM_TASK)

void watchdog_manager_init(void);
void watchdog_manager_report_alive(uint32_t health_flag);
void watchdog_manager_service_if_healthy(void);
uint32_t watchdog_manager_get_flags(void);

#endif
