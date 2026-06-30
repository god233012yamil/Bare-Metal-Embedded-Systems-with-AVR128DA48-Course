#include "system_time.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#ifndef F_CPU
#define F_CPU 24000000UL
#endif

static volatile uint32_t g_system_time_ms = 0U;

/**
 * Initializes TCB0 as a 1 ms system tick source.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_time_init(void)
{
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gv;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = (uint16_t)((F_CPU / 1000UL) - 1UL);
    TCB0.CNT = 0U;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA |= TCB_ENABLE_bm;
}

/**
 * Gets the current millisecond system time.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current system time in milliseconds.
 */
uint32_t system_time_get_ms(void)
{
    uint32_t time_ms;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time_ms = g_system_time_ms;
    }

    return time_ms;
}

/**
 * Checks whether a time deadline has been reached using wrap-safe math.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *     deadline_ms: Deadline time in milliseconds.
 *
 * Returns:
 *     True when now_ms is at or beyond deadline_ms; otherwise false.
 */
bool system_time_deadline_reached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0);
}

/**
 * Handles the TCB0 periodic interrupt and advances the system clock.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    g_system_time_ms++;
}
