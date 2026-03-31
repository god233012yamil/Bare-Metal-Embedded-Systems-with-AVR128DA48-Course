/**
 * @file    adc.c
 * @brief   ADC0 driver — implementation.
 *
 * See adc.h for the public API and design rationale.
 *
 * Register-level notes (AVR128DA48 data sheet, §33):
 *  - VREF.ADC0REF  selects the reference voltage (2.048 V internal).
 *  - ADC0.CTRLA    enables the ADC and sets the resolution.
 *  - ADC0.CTRLB    selects accumulation (none = single sample here).
 *  - ADC0.CTRLC    selects the prescaler.
 *  - ADC0.MUXPOS   selects the positive input channel.
 *  - ADC0.COMMAND  writing STCONV starts a conversion.
 *  - ADC0.INTCTRL  RESRDY bit enables the result-ready interrupt.
 *  - The RES register pair holds the 12-bit result left-adjusted to a
 *    16-bit word; reading RESL first (or using RES as a 16-bit read) is
 *    required to latch the result correctly.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#include "adc.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/* =========================================================================
 * Module-private state
 * ========================================================================= */

/** Most recent 12-bit ADC result; written in ISR, read from main. */
static volatile uint16_t g_adcResult = 0U;

/** Flag: set when a new sample has been captured by the ISR. */
static volatile bool g_resultReady = false;

/* =========================================================================
 * ISR
 * ========================================================================= */

/**
 * @brief ADC0 Result Ready interrupt service routine.
 *
 * Captures the 12-bit result from ADC0.RES (16-bit read of the word
 * register pair).  The hardware clears the RESRDY flag automatically when
 * the result register is read.
 */
ISR(ADC0_RESRDY_vect)
{
    /* Reading the 16-bit RES register (RESL then RESH) clears RESRDY. */
    g_adcResult   = ADC0.RES;
    g_resultReady = true;
}

/* =========================================================================
 * Public function definitions
 * ========================================================================= */

/**
 * @brief Initialises ADC0 and the VREF for ADC use.
 *
 * Configures PD2 (AIN2) as a dedicated analogue input, sets the internal
 * 2.048 V reference, and enables the RESRDY interrupt.
 */
void ADC_Init(void)
{
    /* ---- Configure PD2 as analogue input ---- */
    /* Disable the digital input buffer to reduce power and noise on AIN2.
     * The pin direction defaults to input so no DIR change is needed. */
    PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;

    /* ---- VREF: select 2.048 V internal reference for ADC0 ---- */
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;

    /* ---- ADC0 configuration ---- */

    /* CTRLC: clock prescaler.  Target ADC clock ≤ 2 MHz.
     * F_CPU = 24 MHz; DIV16 → 1.5 MHz.  SAMPCAP = 0 (external cap/high-Z). */
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;

    /* CTRLA: 12-bit resolution, single-ended, ADC enabled. */
    ADC0.CTRLA = ADC_RESSEL_12BIT_gc | ADC_ENABLE_bm;

    /* CTRLB: no accumulation — single sample per trigger. */
    ADC0.CTRLB = ADC_SAMPNUM_NONE_gc;

    /* CTRLD: no initial delay, no sampling delay. */
    ADC0.CTRLD = ADC_INITDLY_DLY0_gc;

    /* MUXPOS: select AIN2 (PD2). */
    ADC0.MUXPOS = ADC_MUX_INPUT;

    /* EVCTRL: enable start-of-conversion via event channel (EVSYS uses this).
     * The ADC will also start on a direct COMMAND write — both paths are OK. */
    ADC0.EVCTRL = ADC_STARTEI_bm;

    /* INTCTRL: enable the Result Ready interrupt. */
    ADC0.INTCTRL = ADC_RESRDY_bm;
}

/**
 * @brief Triggers a single ADC conversion by writing STCONV.
 *
 * Hardware ignores the write if a conversion is already in progress.
 */
void ADC_StartConversion(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief Returns the most recent 12-bit ADC result.
 *
 * @return Conversion result in the range [0, 4095].
 */
uint16_t ADC_GetResult(void)
{
    return g_adcResult;
}

/**
 * @brief Reports whether a fresh result is waiting to be processed.
 *
 * @return true if the ISR has stored a new sample since the last
 *         ADC_ClearResultFlag() call.
 */
bool ADC_IsResultReady(void)
{
    return g_resultReady;
}

/**
 * @brief Clears the result-ready flag.
 *
 * Call after consuming the ADC result to avoid double-processing the same
 * sample.  No atomic protection is needed: the main loop is the only
 * writer of this flag in the clear direction (ISR only sets it).
 */
void ADC_ClearResultFlag(void)
{
    g_resultReady = false;
}
