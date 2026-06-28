/**
 * @file    clkctrl.c
 * @brief   CLKCTRL - Clock Controller driver for AVR128DA48
 *
 * Uses only the #define bit-mask / bit-position macros from the header
 * (_bm, _gm, _gp) to avoid any dependency on enum constants that may
 * be absent in older Microchip Studio toolchain headers.
 *
 * OSCHF raw FRQSEL field values (written into OSCHFCTRLA bits [5:2]):
 *   0x00 =  1 MHz,  0x01 =  2 MHz,  0x02 =  3 MHz,  0x03 =  4 MHz (default)
 *   0x05 =  8 MHz,  0x06 = 12 MHz,  0x07 = 16 MHz,  0x08 = 20 MHz
 *   0x09 = 24 MHz
 * (0x04 is reserved — not a valid FRQSEL value)
 *
 * CCP: OSCHFCTRLA, MCLKCTRLA, MCLKCTRLB are IOREG-protected.
 *      ccp_write_io() from <avr/cpufunc.h> handles the 4-cycle window.
 */

#include "clkctrl.h"

#include <avr/io.h>
#include <avr/cpufunc.h>    /* ccp_write_io() */

/* Raw FRQSEL field values — local #defines, no enum dependency */
#define FRQSEL_RAW_1MHZ     0x00u
#define FRQSEL_RAW_2MHZ     0x01u
#define FRQSEL_RAW_3MHZ     0x02u
#define FRQSEL_RAW_4MHZ     0x03u
#define FRQSEL_RAW_8MHZ     0x05u   /* 0x04 is reserved */
#define FRQSEL_RAW_12MHZ    0x06u
#define FRQSEL_RAW_16MHZ    0x07u
#define FRQSEL_RAW_20MHZ    0x08u
#define FRQSEL_RAW_24MHZ    0x09u
#define PDIV_RAW_10X        0x09u

/* --------------------------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------------------------- */

/**
 * @brief  Map a frequency in Hz to the OSCHFCTRLA register byte.
 *
 *         The raw FRQSEL value is shifted into bits [5:2] using the
 *         CLKCTRL_FRQSEL_gp #define.  AUTOTUNE (bit 0) and RUNSTDBY
 *         (bit 7) are left cleared.
 *
 * @param  freq_hz      Requested frequency in Hz.
 * @param  oschfctrla   Output: value ready to write to OSCHFCTRLA.
 * @return true on success, false if freq_hz is not a valid OSCHF frequency.
 */
static bool freq_to_oschfctrla(uint32_t freq_hz, uint8_t *oschfctrla)
{
    uint8_t raw;

    switch (freq_hz)
    {
        case  1000000UL: raw = FRQSEL_RAW_1MHZ;  break;
        case  2000000UL: raw = FRQSEL_RAW_2MHZ;  break;
        case  3000000UL: raw = FRQSEL_RAW_3MHZ;  break;
        case  4000000UL: raw = FRQSEL_RAW_4MHZ;  break;
        case  8000000UL: raw = FRQSEL_RAW_8MHZ;  break;
        case 12000000UL: raw = FRQSEL_RAW_12MHZ; break;
        case 16000000UL: raw = FRQSEL_RAW_16MHZ; break;
        case 20000000UL: raw = FRQSEL_RAW_20MHZ; break;
        case 24000000UL: raw = FRQSEL_RAW_24MHZ; break;
        default:         return false;
    }

    /* Place the raw value into FRQSEL field bits [5:2].
     * CLKCTRL_FRQSEL_gp is a #define (= 2), always available. */
    *oschfctrla = (uint8_t)((raw << CLKCTRL_FRQSEL_gp) & CLKCTRL_FRQSEL_gm);
    return true;
}

/**
 * @brief  Spin until OSCHF is stable (MCLKSTATUS.OSCHFS == 1).
 */
static void wait_oschf_stable(void)
{
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm))
    {
        /* busy-wait */
    }
}

/**
 * @brief  Spin until the main clock switch completes (MCLKSTATUS.SOSC == 0).
 */
static void wait_clock_switch(void)
{
    while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm)
    {
        /* busy-wait */
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

bool CLKCTRL_init(uint32_t freq_hz)
{
    uint8_t oschfctrla_val;
    uint8_t mclkctrlb_val = 0x00u;

    if (freq_hz == 1200000UL)
    {
        /* Generate 1.2 MHz from the 12 MHz OSCHF and a divide-by-10
         * main-clock prescaler. */
        (void)freq_to_oschfctrla(12000000UL, &oschfctrla_val);
        mclkctrlb_val = (uint8_t)(
            ((PDIV_RAW_10X << CLKCTRL_PDIV_gp) & CLKCTRL_PDIV_gm)
            | CLKCTRL_PEN_bm);
    }
    else if (!freq_to_oschfctrla(freq_hz, &oschfctrla_val))
    {
        return false;   /* unsupported frequency */
    }

    /* Step 1 — Configure OSCHF frequency.
     *   OSCHFCTRLA is CCP/IOREG-protected.
     *   AUTOTUNE (bit 0) and RUNSTDBY (bit 7) remain cleared.            */
    ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA, oschfctrla_val);

    /* Step 2 — Wait for OSCHF to stabilize at the new frequency. */
    wait_oschf_stable();

    /* Step 3 — Select OSCHF as the main clock source.
     *   MCLKCTRLA is CCP/IOREG-protected.
     *   CLKSEL[2:0] = 0x00 → OSCHF.  CLKOUT (PA7) left disabled.        */
    ccp_write_io((void *)&CLKCTRL.MCLKCTRLA,
                 (0x00u << CLKCTRL_CLKSEL_gp) & CLKCTRL_CLKSEL_gm);

    /* Step 4 — Wait for the clock source switch to complete. */
    wait_clock_switch();

    /* Step 5 — Disable the main clock prescaler.
     *   MCLKCTRLB is CCP/IOREG-protected.
     *   PEN = 0 → CPU runs at the full OSCHF frequency.                  */
    ccp_write_io((void *)&CLKCTRL.MCLKCTRLB, mclkctrlb_val);

    return true;
}
