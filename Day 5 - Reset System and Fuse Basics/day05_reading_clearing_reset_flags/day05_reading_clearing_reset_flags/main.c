/*
 * Read reset cause early during startup (AVR128DA48, bare-metal)
 *
 * Why "early":
 * - Reset flags can be overwritten/cleared by later init code.
 * - You want the cause captured before anything else runs.
 *
 * Reset causes are stored in RSTCTRL.RSTFR.
 * Best practice:
 * - Read once into a variable
 * - Clear the flags by writing 1s back to the same bits
 */
#include <avr/io.h>
#include <stdint.h>

static volatile uint8_t g_reset_cause = 0;

/*
 * Capture and clear the reset cause flags.
 *
 * Returns:
 *   Bitmask of RSTCTRL.RSTFR flags at startup.
 */
static inline uint8_t reset_cause_read_and_clear_early(void)
{
    uint8_t flags = RSTCTRL.RSTFR; // Read reset flags (early snapshot)
    RSTCTRL.RSTFR = flags;        // Clear by writing 1s to the set bits
    return flags;
}

int main(void)
{
    /* Capture reset cause as the first thing in main(). */
    g_reset_cause = reset_cause_read_and_clear_early();

    /* Now continue with the rest of your bring-up... */
    // clock_init_...();
    // gpio_init_...();

    while (1)
    {
        /* Your application */
    }
}