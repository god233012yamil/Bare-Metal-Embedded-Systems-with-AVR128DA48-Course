#ifndef SYSTEM_TIME_H_
#define SYSTEM_TIME_H_

#include <stdint.h>
#include <stdbool.h>
#include "status.h"

void system_time_init(void);
uint32_t system_time_get_ms(void);
bool system_time_elapsed(uint32_t start_ms, uint32_t timeout_ms);
void system_time_delay_ms(uint32_t delay_ms);

#endif
