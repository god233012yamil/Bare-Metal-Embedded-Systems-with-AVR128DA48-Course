#include "system_state_machine.h"
#include "led_driver.h"

static system_state_t current_state = SYSTEM_STATE_IDLE;

/**
 * Handles the idle state.
 *
 * Args:
 *     context: Shared system context.
 *
 * Returns:
 *     The next system state.
 */
static system_state_t state_idle(system_context_t *context)
{
    led_driver_set(0U);

    if ((context != 0) && (context->fault_requested != 0U))
    {
        return SYSTEM_STATE_FAULT;
    }

    if ((context != 0) && (context->enabled != 0U))
    {
        return SYSTEM_STATE_ACTIVE;
    }

    return SYSTEM_STATE_IDLE;
}

/**
 * Handles the active state.
 *
 * Args:
 *     context: Shared system context.
 *
 * Returns:
 *     The next system state.
 */
static system_state_t state_active(system_context_t *context)
{
    led_driver_set(1U);

    if ((context != 0) && (context->fault_requested != 0U))
    {
        return SYSTEM_STATE_FAULT;
    }

    if ((context != 0) && (context->enabled == 0U))
    {
        return SYSTEM_STATE_IDLE;
    }

    return SYSTEM_STATE_ACTIVE;
}

/**
 * Handles the fault state.
 *
 * Args:
 *     context: Shared system context.
 *
 * Returns:
 *     The next system state.
 */
static system_state_t state_fault(system_context_t *context)
{
    led_driver_toggle();

    if ((context != 0) && (context->fault_requested == 0U))
    {
        return SYSTEM_STATE_IDLE;
    }

    return SYSTEM_STATE_FAULT;
}

static const state_handler_t state_table[SYSTEM_STATE_COUNT] =
{
    state_idle,
    state_active,
    state_fault
};

/**
 * Initializes the state machine context and state.
 *
 * Args:
 *     context: Shared system context to initialize.
 */
void system_state_machine_init(system_context_t *context)
{
    if (context != 0)
    {
        context->enabled = 0U;
        context->fault_requested = 0U;
        context->button_press_count = 0UL;
    }

    current_state = SYSTEM_STATE_IDLE;
}

/**
 * Runs one state machine step using the function pointer state table.
 *
 * Args:
 *     context: Shared system context.
 */
void system_state_machine_run(system_context_t *context)
{
    if (current_state < SYSTEM_STATE_COUNT)
    {
        current_state = state_table[current_state](context);
    }
    else
    {
        current_state = SYSTEM_STATE_FAULT;
    }
}

/**
 * Gets the current state machine state.
 *
 * Returns:
 *     Current system state.
 */
system_state_t system_state_machine_get_state(void)
{
    return current_state;
}
