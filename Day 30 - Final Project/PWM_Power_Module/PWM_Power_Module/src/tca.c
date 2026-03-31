/**
 * @file    tca.c
 * @brief   TCA0 single-slope PWM driver — implementation.
 *
 * See tca.h for the public API and design rationale.
 *
 * Register-level notes (AVR128DA48 data sheet, §21):
 *  - PORTMUX.TCAROUTEA selects which PORT the WO0-WO5 outputs appear on.
 *    We route TCA0 to PORTD so WO0 → PD0.
 *  - TCA0.SINGLE.CTRLA  enables the timer and sets the clock prescaler.
 *  - TCA0.SINGLE.CTRLB  sets the waveform mode (SINGLESLOPE) and enables
 *    the individual WO outputs (CMP0EN).
 *  - TCA0.SINGLE.PER    sets the top value (period).
 *  - TCA0.SINGLE.CMP0   is the compare register for WO0; the buffered
 *    version CMP0BUF allows glitch-free updates (LUPD workflow).
 *  - In single-slope PWM mode, WO0 goes HIGH at timer reset (CNT=0) and
 *    LOW when CNT == CMP0.  So duty = CMP0 / (PER + 1).
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "tca.h"
#include "config.h"

#include <avr/io.h>

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises TCA0 in single-slope PWM mode, WO0 on PD0.
 */
void TCA_Init(void)
{
    /* ---- Route TCA0 WO0-WO5 to PORTD ---- */
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;

    /* ---- Configure PD0 as push-pull output for WO0 ---- */
    PORTD.DIRSET = PIN0_bm;

    /* ---- TCA0 single-mode configuration ---- */

    /* Stop the timer before configuring (ENABLE = 0). */
    TCA0.SINGLE.CTRLA = 0U;

    /* CTRLB: Single-slope PWM mode, WO0 enabled (CMP0EN = 1). */
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc
                      | TCA_SINGLE_CMP0EN_bm;

    /* CTRLC: No forced output on compare outputs. */
    TCA0.SINGLE.CTRLC = 0U;

    /* CTRLD: No split mode. */
    TCA0.SINGLE.CTRLD = 0U;

    /* Period register: sets the PWM top value. */
    TCA0.SINGLE.PER = TCA0_PERIOD;

    /* Compare 0: start at 0 % duty cycle (output low). */
    TCA0.SINGLE.CMP0    = 0U;
    TCA0.SINGLE.CMP0BUF = 0U;

    /* Interrupt control: not used in this driver. */
    TCA0.SINGLE.INTCTRL = 0U;

    /* CTRLA: set prescaler and enable the timer. */
    TCA0.SINGLE.CTRLA = TCA0_CLKDIV | TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Sets the PWM duty cycle using the double-buffer (glitch-free).
 *
 * @param[in] duty  Compare count in [0, TCA0_PERIOD].  Clamped if larger.
 */
void TCA_SetDuty(uint16_t duty)
{
    /* Clamp to the valid range. */
    if (duty > (uint16_t)TCA0_PERIOD)
    {
        duty = (uint16_t)TCA0_PERIOD;
    }

    /* Write to the buffer register.  The timer copies CMP0BUF → CMP0 on the
     * next timer overflow (LUPD mechanism), preventing output glitches. */
    TCA0.SINGLE.CMP0BUF = duty;
}

/**
 * @brief Returns the currently active (non-buffered) duty cycle count.
 *
 * @return CMP0 register value (0–TCA0_PERIOD).
 */
uint16_t TCA_GetDuty(void)
{
    return TCA0.SINGLE.CMP0;
}

/**
 * @brief Enables the WO0 (PWM) output on PD0.
 */
void TCA_EnableOutput(void)
{
    TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0EN_bm;
}

/**
 * @brief Disables the WO0 (PWM) output; PD0 is released and goes low.
 */
void TCA_DisableOutput(void)
{
    TCA0.SINGLE.CTRLB &= (uint8_t)(~TCA_SINGLE_CMP0EN_bm);
}
