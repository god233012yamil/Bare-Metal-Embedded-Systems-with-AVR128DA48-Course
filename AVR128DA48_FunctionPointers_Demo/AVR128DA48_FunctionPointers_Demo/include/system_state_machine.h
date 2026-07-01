#ifndef SYSTEM_STATE_MACHINE_H
#define SYSTEM_STATE_MACHINE_H

#include <stdint.h>

typedef enum
{
    SYSTEM_STATE_IDLE = 0,
    SYSTEM_STATE_ACTIVE,
    SYSTEM_STATE_FAULT,
    SYSTEM_STATE_COUNT
} system_state_t;

typedef struct
{
    uint8_t enabled;
    uint8_t fault_requested;
    uint32_t button_press_count;
} system_context_t;

typedef system_state_t (*state_handler_t)(system_context_t *context);

void system_state_machine_init(system_context_t *context);
void system_state_machine_run(system_context_t *context);
system_state_t system_state_machine_get_state(void);

#endif
