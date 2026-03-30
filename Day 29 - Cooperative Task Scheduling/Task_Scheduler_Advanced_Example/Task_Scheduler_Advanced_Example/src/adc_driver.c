#include "adc_driver.h"
#include <avr/io.h>

void adc_init(void)
{
    /* Configure PD0 as input, disable digital input buffer to save power */
    PORTD.DIRCLR   = PIN0_bm;
    PORTD.PIN0CTRL  = PORT_ISC_INPUT_DISABLE_gc;

    /* Set ADC reference to VDD */
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;

    /* ADC clock prescaler DIV16 -> 250 kHz @ 4 MHz F_CPU */
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;

    /* 12-bit resolution, right-adjusted result */
    ADC0.CTRLA = ADC_RESSEL_12BIT_gc | ADC_ENABLE_bm;

    /* Select AIN0 (PD0) as positive input, single-ended */
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
}

uint16_t adc_read_blocking(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;

    /* Wait for result ready */
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        ;
    }

    /* Clear flag by writing 1 */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    return ADC0.RES;
}
