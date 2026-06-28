/**
 * @file    clkctrl.h
 * @brief   CLKCTRL - Clock Controller driver for AVR128DA48
 *
 * Supports the internal high-frequency oscillator (OSCHF) as the main clock
 * source. All direct OSCHF frequencies are supported, together with 1.2 MHz
 * generated from the 12 MHz oscillator and the divide-by-10 prescaler.
 *
 * Writes to protected CLKCTRL registers are performed through the
 * Configuration Change Protection (CCP) mechanism.
 *
 * @note    The prescaler is disabled for direct OSCHF frequencies and enabled
 *          only for the 1.2 MHz setting.
 */

#ifndef CLKCTRL_H
#define CLKCTRL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialize the main clock to the requested frequency.
 *
 * Supported frequencies (Hz):
 *   Prescaled OSCHF:  1200000 (12 MHz / 10)
 *   Direct OSCHF   :  1000000, 2000000, 3000000, 4000000, 8000000,
 *                     12000000, 16000000, 20000000, 24000000
 *
 * The function waits for the oscillator to report stable in MCLKSTATUS before
 * returning.
 *
 * @param   freq_hz  Desired CPU frequency in Hz.
 * @return  true  on success.
 *          false if @p freq_hz is not a supported value.
 */
bool CLKCTRL_init(uint32_t freq_hz);

#ifdef __cplusplus
}
#endif

#endif /* CLKCTRL_H */
