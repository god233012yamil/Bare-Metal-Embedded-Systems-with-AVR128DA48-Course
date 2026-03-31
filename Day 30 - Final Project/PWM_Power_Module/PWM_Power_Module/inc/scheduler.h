/**
 * @file    scheduler.h
 * @brief   Cooperative, tick-based task scheduler.
 *
 * Implements a simple super-loop scheduler driven by the RTC Periodic Interrupt
 * Timer (PIT).  The PIT ISR increments a tick counter; the main loop calls
 * SCHED_Run() which checks each registered task's period counter and dispatches
 * it when due.  All tasks must be non-blocking and must return quickly.
 *
 * Design rules:
 *  - No task may call delay_ms() or spin-wait.
 *  - Tasks share CPU cooperatively; longer periods should be used for heavier
 *    work, or work should be broken into smaller state-machine steps.
 *  - The tick interrupt only sets a flag; all task dispatch happens in main
 *    context, keeping interrupt latency minimal.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Configuration
 * ========================================================================= */

/** Maximum number of tasks that can be registered. */
#define SCHED_MAX_TASKS     8U

/* =========================================================================
 * Data types
 * ========================================================================= */

/** Signature of a scheduler task callback. */
typedef void (*TaskFunc_t)(void);

/**
 * @brief Internal task descriptor (not for direct use by application code).
 */
typedef struct
{
    TaskFunc_t  func;       /**< Function pointer to the task body.          */
    uint16_t    period;     /**< Reload period in scheduler ticks.           */
    uint16_t    counter;    /**< Countdown: task fires when this reaches 0.  */
    bool        enabled;    /**< Whether this slot is active.                */
} SchedTask_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes the scheduler and starts the RTC PIT.
 *
 * Must be called once before SCHED_RegisterTask() or SCHED_Run().
 * Configures the RTC PIT to fire at approximately 1 kHz using the internal
 * 32 kHz oscillator divided by 32.
 */
void SCHED_Init(void);

/**
 * @brief Registers a periodic task with the scheduler.
 *
 * @param[in] func    Non-NULL pointer to the task function.
 * @param[in] period  Task period in scheduler ticks (1 tick ≈ 1 ms).
 *                    Must be > 0.
 * @return Task handle (index) on success, or -1 if the task table is full
 *         or arguments are invalid.
 */
int8_t SCHED_RegisterTask(TaskFunc_t func, uint16_t period);

/**
 * @brief Enables or disables a previously registered task.
 *
 * @param[in] handle  Handle returned by SCHED_RegisterTask().
 * @param[in] enable  Pass true to enable, false to disable.
 */
void SCHED_SetTaskEnabled(int8_t handle, bool enable);

/**
 * @brief Main scheduler dispatch loop — call this continuously from main().
 *
 * Checks whether a new tick has arrived (set by the PIT ISR), decrements each
 * task's counter, and calls any task whose counter reaches zero.  If no tick
 * has arrived the function returns immediately, allowing the super-loop to
 * perform other housekeeping or enter a light sleep.
 */
void SCHED_Run(void);

/**
 * @brief Returns the running tick counter (wraps at 65535).
 *
 * Useful for time stamping events in the application layer without exposing
 * the scheduler internals.
 *
 * @return Current tick value.
 */
uint16_t SCHED_GetTick(void);

/**
 * @brief Called by the PIT ISR to advance the scheduler tick.
 *
 * Should not be called from application code.
 */
void SCHED_TickISR(void);

#endif /* SCHEDULER_H_ */
