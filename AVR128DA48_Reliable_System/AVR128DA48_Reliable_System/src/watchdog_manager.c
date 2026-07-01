#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/cpufunc.h>
#include "watchdog_manager.h"

static volatile uint32_t g_health_flags = 0;

/**
 * @brief Configures the watchdog timer used as the last recovery layer.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void watchdog_manager_init(void)
{
    wdt_reset();
    _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_4KCLK_gc);
}

/**
 * @brief Reports that one firmware component made valid progress.
 *
 * Args:
 *     health_flag: Bit mask identifying the component that is alive.
 *
 * Returns:
 *     None.
 */
void watchdog_manager_report_alive(uint32_t health_flag)
{
    g_health_flags |= health_flag;
}

/**
 * @brief Services the watchdog only when all critical components are alive.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void watchdog_manager_service_if_healthy(void)
{
    if ((g_health_flags & HEALTH_ALL_TASKS) == HEALTH_ALL_TASKS)
    {
        g_health_flags = 0;
        wdt_reset();
    }
}

/**
 * @brief Returns the current watchdog health flags for diagnostics.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Current health bit mask.
 */
uint32_t watchdog_manager_get_flags(void)
{
    return g_health_flags;
}
