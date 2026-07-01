#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "config.h"
#include "system_time.h"

static volatile uint32_t g_system_ms = 0;

/**
 * @brief Initializes TCB0 as a 1 ms system tick source.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_time_init(void)
{
    TCB0.CTRLA = 0;
    TCB0.CCMP = (uint16_t)((F_CPU / 1000UL) - 1UL);
    TCB0.CNT = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gv | TCB_ENABLE_bm;
}

/**
 * @brief Returns the monotonic millisecond counter.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current system uptime in milliseconds.
 */
uint32_t system_time_get_ms(void)
{
    uint32_t now_ms;
    uint8_t sreg_copy = SREG;

    cli();
    now_ms = g_system_ms;
    SREG = sreg_copy;

    return now_ms;
}

/**
 * @brief Checks whether a timeout has expired using wrap-safe subtraction.
 *
 * Args:
 *     start_ms: Timestamp captured when the operation started.
 *     timeout_ms: Timeout value in milliseconds.
 *
 * Returns:
 *     true if the timeout has expired, otherwise false.
 */
bool system_time_elapsed(uint32_t start_ms, uint32_t timeout_ms)
{
    return ((system_time_get_ms() - start_ms) >= timeout_ms);
}

/**
 * @brief Delays execution for a bounded number of milliseconds.
 *
 * Args:
 *     delay_ms: Delay time in milliseconds.
 *
 * Returns:
 *     None.
 */
void system_time_delay_ms(uint32_t delay_ms)
{
    uint32_t start_ms = system_time_get_ms();

    while (!system_time_elapsed(start_ms, delay_ms))
    {
        /* This delay is used only in recovery and startup paths. */
    }
}

/**
 * @brief TCB0 interrupt handler used to maintain the system tick.
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
    g_system_ms++;
}
