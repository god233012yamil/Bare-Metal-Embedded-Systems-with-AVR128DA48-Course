/**
 * @file    ac.c
 * @brief   Analog Comparator 0 (AC0) driver — implementation.
 *
 * See ac.h for the public API and design rationale.
 *
 * Register-level notes (AVR128DA48 data sheet, §28):
 *  - VREF.ACREF     sets the reference supplied to the AC DAC.
 *  - AC0.MUXCTRL    selects the positive and negative inputs.
 *  - AC0.DACREF     sets the 8-bit DAC reference level.
 *  - AC0.INTCTRL    enables the edge-triggered interrupt and selects mode.
 *  - AC0.CTRLA      sets hysteresis, power profile, and enables AC0.
 *  - AC0.STATUS     contains the CMPSTATE bit (live output) and the CMPIF flag.
 *
 * The driver exposes a software flag (g_acFlag) that the application layer
 * reads; the ISR sets it and the application clears it via AC_ClearFlag().
 * This decouples the fast interrupt response from the slower cooperative task.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "ac.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/* =========================================================================
 * Module-private state
 * ========================================================================= */

/**
 * Software flag set by the AC ISR.  Volatile because it is written in
 * interrupt context and read in main context.
 */
static volatile bool g_acFlag = false;

/* =========================================================================
 * ISR
 * ========================================================================= */

/**
 * @brief AC0 compare interrupt service routine.
 *
 * Fires on both rising and falling edges of the AC output.  Clears the
 * hardware CMP flag by writing 1 to it, then sets the software flag so the
 * application task can react on its next scheduled run.
 */
ISR(AC0_AC_vect)
{
    /* Clear the hardware interrupt flag (write-1-to-clear). */
    AC0.STATUS = AC_CMPIF_bm;

    /* Signal the application layer. */
    g_acFlag = true;
}

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises AC0 with the default threshold.
 *
 * PD2 is already configured as an analogue input (digital input buffer
 * disabled) by the ADC driver; the AC shares this pin without conflict.
 */
void AC_Init(void)
{
    /* ---- VREF: set AC reference to VDD ---- */
    /* ACREF register controls the AC DAC supply voltage.
     * VDD as reference gives the widest range for the DACREF byte. */
    VREF.ACREF = VREF_REFSEL_VDD_gc;

    /* ---- AC0 mux: positive = AINP0 (PD2), negative = internal DACREF ---- */
    AC0.MUXCTRL = AC_MUX_POS | AC_MUX_NEG;

    /* ---- AC0 DAC reference (threshold) ---- */
    AC0.DACREF = AC_DACREF_DEFAULT;

    /* ---- AC0 interrupt: fire on both edges ---- */
    AC0.INTCTRL = AC_INTMODE_NORMAL_BOTHEDGE_gc | AC_CMP_bm;

    /* ---- AC0 control: small hysteresis, power profile 0, enable ---- */
    AC0.CTRLA = AC_HYSMODE_SMALL_gc
              | AC_POWER_PROFILE0_gc
              | AC_ENABLE_bm;
}

/**
 * @brief Reprograms the AC threshold via the internal DACREF.
 *
 * @param[in] dacref  8-bit reference level (0–255).
 */
void AC_SetThreshold(uint8_t dacref)
{
    AC0.DACREF = dacref;
}

/**
 * @brief Returns the live AC output bit.
 *
 * @return true if AINP0 > DACREF reference.
 */
bool AC_GetOutput(void)
{
    return (bool)(AC0.STATUS & AC_CMPSTATE_bm);
}

/**
 * @brief Reports whether the AC ISR has fired since the last clear.
 *
 * @return true if a threshold crossing has been detected.
 */
bool AC_IsFlagSet(void)
{
    return g_acFlag;
}

/**
 * @brief Clears the application-level AC event flag.
 */
void AC_ClearFlag(void)
{
    g_acFlag = false;
}
