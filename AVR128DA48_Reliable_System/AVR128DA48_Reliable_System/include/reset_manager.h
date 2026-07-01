#ifndef RESET_MANAGER_H_
#define RESET_MANAGER_H_

#include <stdint.h>

uint8_t reset_manager_capture_reason(void);
void reset_manager_software_reset(void);

#endif
