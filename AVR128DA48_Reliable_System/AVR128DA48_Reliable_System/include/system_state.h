#ifndef SYSTEM_STATE_H_
#define SYSTEM_STATE_H_

#include <stdint.h>

typedef enum
{
    SYS_STATE_INIT = 0,
    SYS_STATE_SELF_TEST,
    SYS_STATE_RUN,
    SYS_STATE_FAULT,
    SYS_STATE_RECOVERY,
    SYS_STATE_SAFE_MODE
} system_state_t;

void system_state_init(void);
void system_state_task(void);
system_state_t system_state_get(void);
void system_state_force_fault(void);

#endif
