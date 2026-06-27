#include "system_tick.h"

#include "software_timer.h"

#include <avr/io.h>
#include <util/atomic.h>

#ifndef F_CPU
#define F_CPU 4000000UL
#endif

#define SYSTEM_TICK_HZ 1000UL
#define TCB0_COMPARE_VALUE ((F_CPU / SYSTEM_TICK_HZ) - 1UL)

#if (TCB0_COMPARE_VALUE > UINT16_MAX)
#error "TCB0 compare value does not fit in 16 bits."
#endif

static volatile uint32_t system_milliseconds;

/**
 * @brief Configures TCB0 to generate a 1 ms periodic interrupt.
 */
void system_tick_init(void)
{
    system_milliseconds = 0U;

    TCB0.CTRLA = 0U;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = (uint16_t)TCB0_COMPARE_VALUE;
    TCB0.CNT = 0U;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;
}

/**
 * @brief Returns elapsed milliseconds using an atomic read.
 *
 * Returns:
 *     Milliseconds elapsed since system tick initialization.
 */
uint32_t system_tick_get(void)
{
    uint32_t milliseconds;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        milliseconds = system_milliseconds;
    }

    return milliseconds;
}

/**
 * @brief Returns elapsed milliseconds while already in interrupt context.
 *
 * Returns:
 *     Milliseconds elapsed since system tick initialization.
 */
uint32_t system_tick_get_from_isr(void)
{
    return system_milliseconds;
}

/**
 * @brief Handles one TCB0 system tick interrupt.
 */
void system_tick_irq_handler(void)
{
    system_milliseconds++;
    software_timer_tick_1ms();
}
