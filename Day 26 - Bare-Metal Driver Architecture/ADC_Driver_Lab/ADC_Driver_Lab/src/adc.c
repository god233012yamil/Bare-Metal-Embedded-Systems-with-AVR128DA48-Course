/*
 * adc.c
 *
 * ADC driver implementation for AVR128DA48.
 *
 * Hardware resources used
 *   ADC0       - the only ADC instance on this device
 *   VREF.ADC0REF - selects the reference voltage for ADC0
 *
 * Register symbols are taken directly from ioavr128da48.h via <avr/io.h>.
 * No magic numbers are used; every bit is named with the device-pack enum
 * or bitmask macro.
 *
 * Design notes
 *   - Single-ended, 12-bit mode (ADC_RESSEL_12BIT_gc).
 *   - Blocking (polled) conversions only; no interrupts.
 *   - The VREF module forces the internal reference on while ADC is active.
 *   - The caller is responsible for configuring the GPIO pin's input buffer
 *     before calling ADC_Init().  Example for PD0 (AIN0):
 *         PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
 */

#include "adc.h"
#include <avr/io.h>

/* -----------------------------------------------------------------------
 * Internal helper: map the public adc_ref_t enum to the VREF_REFSEL_*_gc
 * value that must be written to VREF.ADC0REF.
 * ----------------------------------------------------------------------- */
static uint8_t prv_ref_to_gc(adc_ref_t ref)
{
    switch (ref)
    {
        case ADC_REF_1V024:  return VREF_REFSEL_1V024_gc;
        case ADC_REF_2V048:  return VREF_REFSEL_2V048_gc;
        case ADC_REF_4V096:  return VREF_REFSEL_4V096_gc;
        case ADC_REF_2V500:  return VREF_REFSEL_2V500_gc;
        case ADC_REF_VDD:    return VREF_REFSEL_VDD_gc;
        case ADC_REF_VREFA:  return VREF_REFSEL_VREFA_gc;
        default:             return VREF_REFSEL_1V024_gc;
    }
}

/* -----------------------------------------------------------------------
 * ADC_Init
 * ----------------------------------------------------------------------- */
void ADC_Init(const adc_config_t *cfg)
{
    /* 1. Configure the voltage reference for ADC0 via the VREF peripheral. */
    VREF.ADC0REF = prv_ref_to_gc(cfg->reference);

    /* 2. Disable ADC0 before changing configuration registers. */
    ADC0.CTRLA = 0;

    /* 3. Prescaler -> ADC0.CTRLC[3:0] (ADC_PRESC_gm).
     *    Single-ended mode is the power-on default (ADC_CONVMODE_SINGLEENDED_gc = 0).
     *    12-bit resolution is the power-on default (ADC_RESSEL_12BIT_gc = 0). */
    ADC0.CTRLC = (uint8_t)(cfg->prescaler);   /* writes bits [3:0] only */

    /* 4. Accumulation count -> ADC0.CTRLB[2:0] (ADC_SAMPNUM_gm). */
    ADC0.CTRLB = (uint8_t)(cfg->samples);

    /* 5. Select the initial positive-input channel. */
    ADC0.MUXPOS = (uint8_t)(cfg->channel);

    /* 6. Enable ADC0 in 12-bit single-ended mode (RESSEL=0, CONVMODE=0). */
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;
}

/* -----------------------------------------------------------------------
 * ADC_SetChannel
 * ----------------------------------------------------------------------- */
void ADC_SetChannel(adc_channel_t ch)
{
    ADC0.MUXPOS = (uint8_t)ch;
}

/* -----------------------------------------------------------------------
 * ADC_Read
 * Start a conversion on the currently-selected channel and wait for the
 * RESRDY flag in ADC0.INTFLAGS, then return ADC0.RES.
 * ----------------------------------------------------------------------- */
uint16_t ADC_Read(void)
{
    /* Clear any stale result-ready flag before starting. */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    /* Start conversion: write 1 to STCONV in ADC0.COMMAND. */
    ADC0.COMMAND = ADC_STCONV_bm;

    /* Poll until the result-ready flag is set. */
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        /* busy-wait */
    }

    /* Reading ADC0.RES clears RESRDY automatically on AVR-Dx. */
    return ADC0.RES;
}

/* -----------------------------------------------------------------------
 * ADC_ReadChannel
 * ----------------------------------------------------------------------- */
uint16_t ADC_ReadChannel(adc_channel_t ch)
{
    ADC_SetChannel(ch);
    return ADC_Read();
}

/* -----------------------------------------------------------------------
 * ADC_ToMillivolts
 * result_mV = (raw * vref_mV) / 4095
 * Uses 32-bit arithmetic to avoid overflow.
 * ----------------------------------------------------------------------- */
uint32_t ADC_ToMillivolts(uint16_t raw, uint32_t vref_mv)
{
    return ((uint32_t)raw * vref_mv) / 4095UL;
}

/* -----------------------------------------------------------------------
 * ADC_Disable
 * ----------------------------------------------------------------------- */
void ADC_Disable(void)
{
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
}

/* -----------------------------------------------------------------------
 * ADC_Enable
 * ----------------------------------------------------------------------- */
void ADC_Enable(void)
{
    ADC0.CTRLA |= ADC_ENABLE_bm;
}
