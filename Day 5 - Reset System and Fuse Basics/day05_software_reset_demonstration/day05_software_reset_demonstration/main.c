/*
 * AVR128DA48 - Software Reset Demonstration
 *
 * Goal:
 * - Run firmware normally
 * - Wait ~500 ms using software tick counting
 * - Trigger a software reset
 *
 * Notes:
 * - No timers
 * - No delay functions
 * - Reset is triggered via RSTCTRL.SWRR
 */

#include <avr/io.h>
#include <stdint.h>

/* -------------------------------------------------- */
/* Protected register write helper                    */
/* -------------------------------------------------- */
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;
    *addr = value;
}

/* -------------------------------------------------- */
/* Simple software delay using CPU execution          */
/* -------------------------------------------------- */

/*
 * Crude delay loop.
 * Duration depends on CPU clock and compiler optimization.
 * Intended for demonstration only.
 */
static void delay_software_ticks(uint32_t ticks)
{
    while (ticks--)
    {
        __asm__ __volatile__("nop");
    }
}

/* -------------------------------------------------- */
/* Trigger a software reset                           */
/* -------------------------------------------------- */
static void software_reset(void)
{
    /* Writing 1 to SWRR triggers an immediate reset */
    ccp_write_io(&RSTCTRL.SWRR, 1);

    /* Execution never reaches here */
    while (1)
    {
    }
}

/* -------------------------------------------------- */
/* Main application                                   */
/* -------------------------------------------------- */

int main(void)
{
    /*
     * Approximate 500 ms delay.
     * This value is empirical and depends on clock and optimization.
     *
     * Students are encouraged to change it and observe behavior.
     */
    delay_software_ticks(3000000UL);

    /* Trigger software reset */
    software_reset();

    /* Not reached */
    while (1)
    {
    }
}