/**
 * @file adc_hw_accum_example.c
 * @brief Simple ADC0 hardware accumulation example (AVR128DA48).
 *
 * This example demonstrates ADC hardware accumulation as a better alternative
 * to manual software averaging.
 *
 * What it does:
 * - Samples AIN0 multiple times using the ADC hardware accumulator
 * - Returns the accumulated result so you can compute an average
 *
 * Notes:
 * - The exact register/bit names for accumulation can vary slightly by device pack.
 * - If your pack uses different names for ADC accumulation control, adjust the
 *   ADC0.CTRLB configuration section.
 */

#include <avr/io.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU (24000000UL)
#endif

#define ADC_INPUT_MUXPOS   ADC_MUXPOS_AIN0_gc

/* Choose accumulation samples: 16 samples is a good starter */
#define ADC_ACC_SAMPLES    16u

/**
 * @brief Initialize ADC0 for single-ended sampling with hardware accumulation.
 */
static void adc0_init_hw_accum(void)
{
    /* Disable ADC before configuration */
    ADC0.CTRLA = 0;

    // Configure the ADC clock prescaler (adjust as needed for your application)
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;

    // Set reference voltage (adjust as needed; using VDD in this example)
    VREF.ADC0REF = VREF_REFSEL_VDD_gc; 

    /* Select input channel */
    ADC0.MUXPOS = ADC_INPUT_MUXPOS;

    /*
     * Enable hardware accumulation.
     *
     * Device-pack dependent:
     * - Many AVR Dx headers provide ADC_SAMPNUM_* enums to select accumulation count.
     * - Example values: ADC_SAMPNUM_ACC16_gc, ADC_SAMPNUM_ACC32_gc, etc.
     *
     * If your headers differ, search for "SAMPNUM" in io.h.
     */
    ADC0.CTRLB = ADC_SAMPNUM_ACC16_gc; /* Accumulate 16 samples */

    /* Enable ADC */
    ADC0.CTRLA = ADC_ENABLE_bm;
}

/**
 * @brief Perform one accumulated conversion and return the accumulated result.
 *
 * @return Accumulated ADC result (sum of N samples).
 *
 * How to compute average:
 * - avg = accum / N
 */
static uint16_t adc0_read_accumulated(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;

    /* Wait for result ready */
    while ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0)
    {
        /* Busy wait (for demo only) */
    }

    /* Read result (accumulated) */
    uint16_t accum = ADC0.RES;

    /* Clear flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    return accum;
}

int main(void)
{
    adc0_init_hw_accum();

    while (1)
    {
        /* Read accumulated result */
        uint16_t accum = adc0_read_accumulated();

        /* Compute average for 16 samples (if using ACC16) */
        uint16_t avg = (uint16_t)(accum / 16u);

        /* Place breakpoints here to inspect accum and avg */
        (void)avg;
    }
}