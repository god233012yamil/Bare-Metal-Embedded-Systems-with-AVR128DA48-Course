#include <avr/io.h>
#include "config.h"
#include "system_state.h"
#include "fault.h"
#include "control.h"
#include "system_time.h"
#include "watchdog_manager.h"
#include "reset_manager.h"

static system_state_t g_system_state = SYS_STATE_INIT;
static bool g_forced_fault_requested = false;
static uint32_t g_recovery_start_ms = 0;
static uint32_t g_last_heartbeat_ms = 0;

static status_t system_self_test(void);
static status_t system_recovery_attempt(void);
static void heartbeat_task(void);

/**
 * @brief Initializes the main system state machine.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_state_init(void)
{
    if (fault_safe_mode_required())
    {
        g_system_state = SYS_STATE_SAFE_MODE;
    }
    else
    {
        g_system_state = SYS_STATE_INIT;
    }
}

/**
 * @brief Runs the main non-blocking system state machine.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_state_task(void)
{
    switch (g_system_state)
    {
        case SYS_STATE_INIT:
            g_system_state = SYS_STATE_SELF_TEST;
            break;

        case SYS_STATE_SELF_TEST:
            if (system_self_test() == STATUS_OK)
            {
                fault_mark_boot_successful();
                g_system_state = SYS_STATE_RUN;
            }
            else
            {
                fault_report(FAULT_INVALID_STATE);
                g_system_state = SYS_STATE_FAULT;
            }
            break;

        case SYS_STATE_RUN:
            heartbeat_task();

            if (g_forced_fault_requested)
            {
                g_forced_fault_requested = false;
                fault_report(FAULT_FORCED_BY_BUTTON);
                g_system_state = SYS_STATE_FAULT;
            }
            break;

        case SYS_STATE_FAULT:
            outputs_set_safe_state();
            fault_save_snapshot((uint8_t)g_system_state);
            g_recovery_start_ms = system_time_get_ms();
            g_system_state = SYS_STATE_RECOVERY;
            break;

        case SYS_STATE_RECOVERY:
            if (!system_time_elapsed(g_recovery_start_ms, SYSTEM_RECOVERY_DELAY_MS))
            {
                break;
            }

            if (system_recovery_attempt() == STATUS_OK)
            {
                g_system_state = SYS_STATE_RUN;
            }
            else
            {
                fault_report(FAULT_RECOVERY_FAILED);
                fault_save_snapshot((uint8_t)g_system_state);
                reset_manager_software_reset();
            }
            break;

        case SYS_STATE_SAFE_MODE:
            outputs_set_safe_state();
            heartbeat_task();
            break;

        default:
            fault_report(FAULT_INVALID_STATE);
            g_system_state = SYS_STATE_FAULT;
            break;
    }

    watchdog_manager_report_alive(HEALTH_SYSTEM_TASK);
}

/**
 * @brief Returns the current system state.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current system state value.
 */
system_state_t system_state_get(void)
{
    return g_system_state;
}

/**
 * @brief Requests a controlled fault to test recovery behavior.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_state_force_fault(void)
{
    g_forced_fault_requested = true;
}

/**
 * @brief Executes startup self-test checks.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     STATUS_OK if startup checks pass, otherwise an error code.
 */
static status_t system_self_test(void)
{
    if ((FAULT_BUTTON_PORT.IN & FAULT_BUTTON_PIN) == 0U)
    {
        return STATUS_OK;
    }

    return STATUS_OK;
}

/**
 * @brief Attempts to recover from a controlled fault.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     STATUS_OK if recovery succeeds, otherwise an error code.
 */
static status_t system_recovery_attempt(void)
{
    outputs_set_safe_state();
    return STATUS_OK;
}

/**
 * @brief Toggles the heartbeat LED at a fixed period.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void heartbeat_task(void)
{
    uint32_t now_ms = system_time_get_ms();

    if ((now_ms - g_last_heartbeat_ms) >= HEARTBEAT_PERIOD_MS)
    {
        g_last_heartbeat_ms = now_ms;
        LED_HEARTBEAT_PORT.OUT ^= LED_HEARTBEAT_PIN;
    }
}
