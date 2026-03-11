/**
 * @file main.c
 * @brief ADC lab: Read potentiometer value on AIN0.
 */
#include <avr/io.h>
#include <stdint.h>

/**
 * @brief Initialize ADC0 for single-ended 12-bit conversion.
 */
static void ADC0_init(void)
{
    // Disable ADC before configuration
    ADC0.CTRLA = 0; 
    
    // Select reference 
    // Enable reference always ON for ADC0 
    // Internal 4.096V reference 
    VREF.ADC0REF = 1 << VREF_ALWAYSON_bp 
	      		 | VREF_REFSEL_4V096_gc; 

    // ADC input pin 0 
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc; 

    // No accumulation 
    ADC0.CTRLB = ADC_SAMPNUM_NONE_gc; 

    // Prescaler = CLK_PER / 16
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;

    // 12-bit mode 
    ADC0.CTRLA |= ADC_RESSEL_12BIT_gc;

    // Single-ended conversion mode 
    ADC0.CTRLA |= 0 << ADC_CONVMODE_bp; 

    // Enable ADC 
    ADC0.CTRLA |= 1 << ADC_ENABLE_bp;
}

/**
 * @brief Perform single ADC conversion on AIN0.
 */
static uint16_t ADC0_read(void)
{
    // Select channel 
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    // Start conversion 
    ADC0.COMMAND = ADC_STCONV_bm;

    // Wait for result ready 
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm))
    {
        // Do nothing
    }

    // Clear flag 
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    // Return result
    return ADC0.RES;
}

/**
 * @brief Program entry point.
 */
int main(void)
{
    uint16_t adc_value;

    // Initialize ADC and continuously read potentiometer value. 
    ADC0_init();

    while (1)
    {
        adc_value = ADC0_read();

        /* Place breakpoint here and observe adc_value */
    }
}