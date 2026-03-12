/**
 * @file adc_freerun_example.c
 * @brief Simple ADC0 Free-Running Mode example (AVR128DA48).
 *
 * What this example demonstrates:
 * - ADC runs continuously (free-running mode)
 * - Each completed conversion triggers an interrupt
 * - ISR stores the latest ADC result in a global variable
 * - Main loop never blocks waiting for ADC
 *
 * Target:
 * - MCU: AVR128DA48
 * - IDE: Microchip Studio / Atmel Studio 7
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU (24000000UL)
#endif

/* Global variable updated by ADC ISR */
volatile uint16_t g_latest_adc = 0;

/**
 * @brief Initialize ADC0 in free-running mode on AIN0.
 *
 * Configuration:
 * - Reference: VDD
 * - Prescaler: DIV4 (adjust as needed)
 * - Input: AIN0
 * - Free-running mode enabled
 * - Result Ready interrupt enabled
 */
static void adc0_init_freerun(void)
{
    /* Disable ADC before configuration */
    ADC0.CTRLA = 0;

    /* Select reference and prescaler */
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV4_gc;

    /* Select input channel (example: AIN0) */
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    /* Enable free-running mode */
#ifdef ADC_FREERUN_bm
    ADC0.CTRLA |= ADC_FREERUN_bm;
#else
    /* Some device packs use FREERUN bit in CTRLB */
#ifdef ADC_FREERUN_bm
    ADC0.CTRLB |= ADC_FREERUN_bm;
#endif
#endif

    /* Enable Result Ready interrupt */
    ADC0.INTCTRL = ADC_RESRDY_bm;

    /* Clear any pending flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    /* Enable ADC */
    ADC0.CTRLA |= ADC_ENABLE_bm;

    /* Start first conversion (free-running continues automatically) */
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief ADC0 Result Ready ISR.
 *
 * This ISR executes every time a conversion completes.
 * It reads the ADC result and stores it in a global variable.
 */
ISR(ADC0_RESRDY_vect)
{
    /* Read conversion result */
    g_latest_adc = ADC0.RES;

    /* Clear result ready flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

int main(void)
{
    /* Initialize ADC in free-running mode */
    adc0_init_freerun();

    /* Enable global interrupts */
    sei();

    while (1)
    {
        /*
         * Main loop does not block on ADC.
         * g_latest_adc is continuously updated in background.
         *
         * You can:
         * - Apply filtering here
         * - Send value over UART
         * - Convert to voltage
         */

        uint16_t value = g_latest_adc;

        /* Example: convert to voltage (VDD reference assumed 5.0V) */
        float voltage = (value / 1023.0f) * 5.0f;

        (void)voltage;
    }
}