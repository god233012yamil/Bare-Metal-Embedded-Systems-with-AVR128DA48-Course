/*
 * adc.c  --  ADC driver implementation, AVR128DA48
 *
 * Peripheral : ADC0  (0x0600)
 * Reference  : VREF.ADC0REF
 * All register symbols from ioavr128da48.h via <avr/io.h>.
 */

#include "adc.h"
#include <avr/io.h>

static uint8_t prv_ref_to_gc(adc_ref_t ref)
{
    switch (ref) {
        case ADC_REF_1V024: return VREF_REFSEL_1V024_gc;
        case ADC_REF_2V048: return VREF_REFSEL_2V048_gc;
        case ADC_REF_4V096: return VREF_REFSEL_4V096_gc;
        case ADC_REF_2V500: return VREF_REFSEL_2V500_gc;
        case ADC_REF_VDD:   return VREF_REFSEL_VDD_gc;
        case ADC_REF_VREFA: return VREF_REFSEL_VREFA_gc;
        default:            return VREF_REFSEL_1V024_gc;
    }
}

void ADC_Init(const adc_config_t *cfg)
{
    VREF.ADC0REF = prv_ref_to_gc(cfg->reference);
    ADC0.CTRLA   = 0;
    ADC0.CTRLC   = (uint8_t)(cfg->prescaler);
    ADC0.CTRLB   = (uint8_t)(cfg->samples);
    ADC0.MUXPOS  = (uint8_t)(cfg->channel);
    ADC0.CTRLA   = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;
}

void ADC_SetChannel(adc_channel_t ch)
{
    ADC0.MUXPOS = (uint8_t)ch;
}

uint16_t ADC_Read(void)
{
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    ADC0.COMMAND  = ADC_STCONV_bm;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {}
    return ADC0.RES;
}

uint16_t ADC_ReadChannel(adc_channel_t ch)
{
    ADC_SetChannel(ch);
    return ADC_Read();
}

uint32_t ADC_ToMillivolts(uint16_t raw, uint32_t vref_mv)
{
    return ((uint32_t)raw * vref_mv) / 4095UL;
}

void ADC_Disable(void)
{
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
}

void ADC_Enable(void)
{
    ADC0.CTRLA |= ADC_ENABLE_bm;
}
