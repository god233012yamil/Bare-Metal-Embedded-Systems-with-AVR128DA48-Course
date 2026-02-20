/*
 * AVR128DA48 - Watchdog reset demonstration (bare-metal)
 *
 * Goal:
 * - Intentionally trigger a watchdog reset
 * - Read reset cause early and confirm WDT reset flag
 *
 * Notes:
 * - On AVR DA-series, WDT is controlled by WDT.* registers.
 * - WDT configuration is protected (CCP).
 * - After reset, the reset cause is available in RSTCTRL.RSTFR.
 */
#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

/* Store reset cause for debugging (watch in debugger) */
static volatile uint8_t g_reset_cause = 0;

/* Protected register write helper */
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;
    *addr = value;
}

/*
 * Capture and clear reset flags as early as possible.
 *
 * Returns:
 *   Bitmask of reset flags present at startup.
 */
static inline uint8_t reset_cause_read_and_clear_early(void)
{
    uint8_t flags = RSTCTRL.RSTFR;  // Read
    RSTCTRL.RSTFR = flags;          // Clear by writing back the set bits
    return flags;
}

/*
 * Enable watchdog with a short timeout.
 * Intentionally do NOT feed it afterward to force a reset.
 *
 * Timeout selection:
 * - PER_1KCLK is a common short period choice.
 * - Exact time depends on WDT clock source (Datasheet), but it will reset quickly.
 */
static void wdt_enable_short_timeout(void)
{
	// Disable global interrupts.
	cli();
	// Reset Watchdog Timer.
	__asm__ __volatile__("wdr");
	// Enable WDT, set a short period
	ccp_write_io(&WDT.CTRLA, WDT_WINDOW_OFF_gc | WDT_PERIOD_1KCLK_gc);
	// Enable global interrupts.
	sei();
}

/*
 *	Application entry point
 */
int main(void)
{
    /* Step 1: read reset cause right away */
    g_reset_cause = reset_cause_read_and_clear_early();

    /* If the previous reset was caused by WDT, this bit should be set */
    /* Typical name in headers is RSTCTRL_WDRF_bm (watchdog reset flag). */

    /* Step 2: enable watchdog */
    wdt_enable_short_timeout();

    /* Step 3: hang forever WITHOUT clearing the WDT -> watchdog reset occurs */
    while (1)
    {
        /* Intentionally do nothing.
         * No wdt_reset(), no feeding, no delays.
         * The MCU will reset when the WDT expires.
         */
    }
}