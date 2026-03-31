/**
 * @file    evsys.c
 * @brief   Event System (EVSYS) configuration — implementation.
 *
 * See evsys.h for design overview and channel mapping.
 *
 * Register-level notes (AVR128DA48 data sheet, §14):
 *  - EVSYS.CHANNELn selects the generator (event source) for channel n.
 *  - EVSYS.USERxxx  connects a peripheral's event input to a numbered channel.
 *    Write the channel number + 1 to the USER register (0 = disconnected).
 *  - Channels 0–3 are "synchronous" (edge-detection capable).
 *    Channels 4–9 are "asynchronous".
 *  - AC0_OUT is available on Channel 0 generator (EVSYS_CHANNEL0_AC0_OUT_gc).
 *  - USERADC0START connects ADC0's start-of-conversion input to a channel.
 *
 * The EVSYS does not require interrupts or CPU intervention after init.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "evsys.h"
#include "config.h"

#include <avr/io.h>

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Configures EVSYS: AC0_OUT → ADC0 start-of-conversion.
 *
 * After this call, every AC0 output edge will automatically trigger an ADC0
 * conversion.  This is in addition to any conversion started by the
 * scheduler task via ADC_StartConversion().
 */
void EVSYS_Init(void)
{
    /* ---- Channel 0: Generator = AC0 output ---- */
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_AC0_OUT_gc;

    /* ---- User: ADC0 START connected to Channel 0 ---- */
    /* USER registers accept a value of (channel_index + 1); 0 = disconnected.
     * Channel 0 → write value 0x01. */
    EVSYS.USERADC0START = 0x01U;
}
