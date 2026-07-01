#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <stdint.h>

typedef void (*command_handler_t)(const char *args, void *context);

typedef struct
{
    const char *command;
    command_handler_t handler;
} command_entry_t;

typedef struct
{
    const command_entry_t *table;
    uint8_t count;
    void *context;
} command_dispatcher_t;

void command_dispatcher_init(command_dispatcher_t *dispatcher,
                             const command_entry_t *table,
                             uint8_t count,
                             void *context);

uint8_t command_dispatcher_process(command_dispatcher_t *dispatcher,
                                   const char *line);

#endif
