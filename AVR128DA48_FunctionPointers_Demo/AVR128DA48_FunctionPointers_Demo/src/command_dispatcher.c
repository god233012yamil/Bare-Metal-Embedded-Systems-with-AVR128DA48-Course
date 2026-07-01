#include <string.h>
#include "command_dispatcher.h"

/**
 * Initializes a command dispatcher.
 *
 * Args:
 *     dispatcher: Dispatcher object to initialize.
 *     table: Command table containing command strings and handler functions.
 *     count: Number of entries in the command table.
 *     context: User context passed to matched command handlers.
 */
void command_dispatcher_init(command_dispatcher_t *dispatcher,
                             const command_entry_t *table,
                             uint8_t count,
                             void *context)
{
    if (dispatcher == 0)
    {
        return;
    }

    dispatcher->table = table;
    dispatcher->count = count;
    dispatcher->context = context;
}

/**
 * Processes a command line using a dispatch table.
 *
 * Args:
 *     dispatcher: Dispatcher object.
 *     line: Null-terminated command line.
 *
 * Returns:
 *     1 if a command was found and executed, otherwise 0.
 */
uint8_t command_dispatcher_process(command_dispatcher_t *dispatcher,
                                   const char *line)
{
    uint8_t i;

    if ((dispatcher == 0) || (dispatcher->table == 0) || (line == 0))
    {
        return 0U;
    }

    for (i = 0U; i < dispatcher->count; i++)
    {
        if ((dispatcher->table[i].command != 0) &&
            (dispatcher->table[i].handler != 0) &&
            (strcmp(line, dispatcher->table[i].command) == 0))
        {
            dispatcher->table[i].handler(0, dispatcher->context);
            return 1U;
        }
    }

    return 0U;
}
