/**
 * @file main.c
 * @brief ADC triggered by TCB0 via Event System.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Global variable to store the latest ADC value
volatile uint16_t g_adc_value;

/**
 * @brief Initialize TCB0 to generate periodic overflow at 1 kHz.
 */
static void TCB0_init(void)
{
    // Disable TCB0 before configuration 
	TCB0.CTRLA &= ~(1 << TCB_ENABLE_bp);

    // Periodic interrupt mode 
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;

    // Set TCB0 to periodic interrupt mode with a period of 1 ms (1 kHz)
    // F_CPU = 24 MHz
    // CLK_PER = F_CPU / TCB0_Prescaler = 12 MHz
    // TCB0 Period = (CCMP + 1) / CLK_PER 
    // CCMP = (TCB0 Period * CLK_PER) - 1 = (0.001 s * 12,000,000 Hz) - 1
    // CCMP = 12000 - 1 = 11999
    TCB0.CCMP = 11999;

    // Enable timer
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

/**
 * @brief Configure EVSYS Channel 0 to use TCB0 overflow.
 */
static void EVSYS_init(void)
{
    // Configure EVSYS Channel 0 to use TCB0 overflow as 
    // the event source
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCB0_OVF_gc;

    // Configure ADC0 to use event channel 0 as the 
    // start trigger
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;
}

/**
 * @brief Initialize ADC0 for event-triggered sampling.
 */
static void ADC0_init(void)
{
    // Set 2.048 V reference for ADC0 in the VREF peripheral
    VREF.ADC0REF   = VREF_REFSEL_2V048_gc;

    // Select input channel AIN0 for ADC0
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    // Set ADC0 clock prescaler to divide by 16
    ADC0.CTRLC = ADC_PRESC_DIV16_gc;

    // Enable ADC0 result ready interrupt
    ADC0.INTCTRL = ADC_RESRDY_bm;

    // Enables the event input as trigger for starting 
    // an ADC conversion
    ADC0.EVCTRL = ADC_STARTEI_bm;
    
    // Enable ADC0 with 12-bit resolution
    ADC0.CTRLA = ADC_ENABLE_bm
               | ADC_RESSEL_12BIT_gc;  
}

/**
 * @brief ADC0 Result-Ready ISR
 */
ISR(ADC0_RESRDY_vect)
{
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    g_adc_value = ADC0.RES;
}

/**
 * @brief Application entry point
 */
int main(void)
{
    // Initialize TCB0 to generate overflow events at 1 kHz
    TCB0_init();

    // Configure EVSYS to route TCB0 overflow events to ADC0 start trigger
    EVSYS_init();
    
    // Initialize ADC0. ADC0 will start sampling automatically at 1 kHz
    ADC0_init();

    // Enable global interrupts
    sei();

    while (1)
    {
        /* ADC sampled at exactly 1 kHz by hardware */
    }
}