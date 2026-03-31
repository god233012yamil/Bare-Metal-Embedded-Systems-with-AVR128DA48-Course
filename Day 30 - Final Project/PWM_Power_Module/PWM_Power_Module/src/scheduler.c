/**
 * @file    scheduler.c
 * @brief   Cooperative, tick-based task scheduler — implementation.
 *
 * The RTC PIT is clocked from the internal 32 kHz oscillator and configured
 * for a 32-cycle period, yielding a tick rate of 32768 / 32 = 1024 Hz
 * (≈ 0.977 ms per tick).
 *
 * Tick delivery:
 *  1. PIT ISR fires → sets g_tickFlag and increments g_tickCount.
 *  2. SCHED_Run() checks g_tickFlag in the super-loop.
 *  3. Each task's counter is decremented; tasks with counter == 0 are called
 *     and their counter reloaded to their period.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "scheduler.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

/* =========================================================================
 * Module-private state
 * ========================================================================= */

/** Registered task table. */
static SchedTask_t  g_tasks[SCHED_MAX_TASKS];

/** Number of occupied task slots. */
static uint8_t      g_taskCount = 0U;

/**
 * Flag set by the PIT ISR each tick; consumed (cleared) by SCHED_Run().
 * Declared volatile because it is written in ISR context and read in main.
 */
static volatile bool g_tickFlag = false;

/**
 * Running tick counter — wraps at 65535.
 * Incremented in ISR; read (snapshot) in main via SCHED_GetTick().
 */
static volatile uint16_t g_tickCount = 0U;

/* =========================================================================
 * ISR
 * ========================================================================= */

/**
 * @brief RTC Periodic Interrupt Timer ISR.
 *
 * Fires at approximately 1024 Hz.  Keeps execution as short as possible:
 * only sets the tick flag and increments the counter.  All dispatch logic
 * lives in SCHED_Run() (main context).
 */
ISR(RTC_PIT_vect)
{
    /* Clear the interrupt flag (write 1 to PITIF). */
    RTC.PITINTFLAGS = RTC_PI_bm;

    g_tickFlag  = true;
    g_tickCount = (uint16_t)(g_tickCount + 1U);
}

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises the scheduler and the RTC PIT.
 *
 * Waits for any pending RTC synchronisation before writing registers,
 * as required by the AVR-Dx data sheet (§22).
 */
void SCHED_Init(void)
{
    uint8_t i;

    /* Clear the task table. */
    for (i = 0U; i < SCHED_MAX_TASKS; i++)
    {
        g_tasks[i].func    = NULL;
        g_tasks[i].period  = 0U;
        g_tasks[i].counter = 0U;
        g_tasks[i].enabled = false;
    }
    g_taskCount = 0U;
    g_tickFlag  = false;
    g_tickCount = 0U;

    /* ---- RTC / PIT configuration ---- */

    /* Select the internal 32 kHz oscillator as the RTC clock source. */
    while (RTC.STATUS & RTC_CTRLABUSY_bm) { /* wait for sync */ }
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* Configure the PIT: period = 32 cycles → 32768/32 = 1024 Hz. */
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) { /* wait for sync */ }
    RTC.PITCTRLA = RTC_PERIOD_CYC32_gc | RTC_PITEN_bm;

    /* Enable the PIT interrupt. */
    RTC.PITINTCTRL = RTC_PI_bm;
}

/**
 * @brief Registers a periodic task.
 *
 * @param[in] func    Task callback (must not be NULL).
 * @param[in] period  Task period in ticks (must be > 0).
 * @return Task handle (0–SCHED_MAX_TASKS-1), or -1 on failure.
 */
int8_t SCHED_RegisterTask(TaskFunc_t func, uint16_t period)
{
    if ((func == NULL) || (period == 0U) || (g_taskCount >= SCHED_MAX_TASKS))
    {
        return -1;
    }

    g_tasks[g_taskCount].func    = func;
    g_tasks[g_taskCount].period  = period;
    g_tasks[g_taskCount].counter = period;   /* fire after first full period */
    g_tasks[g_taskCount].enabled = true;

    return (int8_t)(g_taskCount++);
}

/**
 * @brief Enables or disables a registered task by handle.
 *
 * @param[in] handle  Task handle from SCHED_RegisterTask().
 * @param[in] enable  true = enabled, false = disabled.
 */
void SCHED_SetTaskEnabled(int8_t handle, bool enable)
{
    if ((handle >= 0) && ((uint8_t)handle < g_taskCount))
    {
        g_tasks[(uint8_t)handle].enabled = enable;
    }
}

/**
 * @brief Dispatcher — call from the super-loop in main().
 *
 * Checks for a new tick (atomic read-clear of g_tickFlag), then iterates
 * the task table and dispatches any task whose countdown reaches zero.
 */
void SCHED_Run(void)
{
    uint8_t i;
    bool    tick;

    /* Atomically capture and clear the tick flag. */
    cli();
    tick       = g_tickFlag;
    g_tickFlag = false;
    sei();

    if (!tick)
    {
        return;     /* No new tick — nothing to do. */
    }

    for (i = 0U; i < g_taskCount; i++)
    {
        if (!g_tasks[i].enabled)
        {
            continue;
        }

        if (g_tasks[i].counter > 0U)
        {
            g_tasks[i].counter--;
        }

        if (g_tasks[i].counter == 0U)
        {
            g_tasks[i].counter = g_tasks[i].period;    /* reload */
            g_tasks[i].func();                          /* dispatch */
        }
    }
}

/**
 * @brief Returns a snapshot of the running tick counter.
 *
 * @return Current tick count (wraps at 65535).
 */
uint16_t SCHED_GetTick(void)
{
    uint16_t t;

    cli();
    t = g_tickCount;
    sei();

    return t;
}

/**
 * @brief Called from the PIT ISR — exposed so the ISR vector can live here.
 *
 * The actual ISR is defined in this file; this stub exists only to satisfy
 * the header declaration for documentation purposes.
 */
void SCHED_TickISR(void)
{
    /* Body intentionally empty — the real work is in ISR(RTC_PIT_vect). */
}
