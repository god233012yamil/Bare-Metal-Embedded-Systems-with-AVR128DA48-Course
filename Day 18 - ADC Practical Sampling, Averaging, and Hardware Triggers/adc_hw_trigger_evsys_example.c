/**
 * @file adc_hw_trigger_evsys_example.c
 * @brief Hardware-triggered ADC sampling using Timer -> EVSYS -> ADC on AVR128DA48.
 *
 * What this example demonstrates:
 * - A timer (TCA0 overflow) generates a precise periodic event (sample rate).
 * - EVSYS routes that event to the ADC start input.
 * - ADC performs a conversion on every timer event (no polling in main).
 * - ADC result-ready interrupt stores the latest sample in a global variable.
 *
 * Notes:
 * - EVSYS generator/user enum names can vary slightly with device pack versions.
 * - This file uses #if defined(...) guards where names may differ.
 * - Assumes F_CPU is the peripheral clock for TCA0 and ADC.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU (24000000UL)
#endif

/* Desired sample rate (Hz) */
#define ADC_SAMPLE_HZ (1000UL)

/* Latest ADC sample, updated in ISR */
volatile uint16_t g_latest_adc_sample = 0;

/**
 * @brief Initialize TCA0 to overflow at ADC_SAMPLE_HZ and generate an event.
 *
 * We use TCA0 in normal mode (not PWM) so overflow occurs at a fixed rate.
 * Overflow frequency = F_CPU / (prescaler * (PER + 1))
 */
static void tca0_init_overflow_event(uint32_t sample_hz)
{
    uint32_t top;

    /* Disable TCA0 before configuration */
    TCA0.SINGLE.CTRLA = 0;

    /* Normal mode (default WGMODE = NORMAL) */
    TCA0.SINGLE.CTRLB = 0;

    /* Protect against divide by zero */
    if (sample_hz == 0u)
    {
        sample_hz = 1u;
    }

    /* Using DIV1 prescaler for simplicity */
    top = (uint32_t)(F_CPU / sample_hz);
    if (top != 0u)
    {
        top -= 1u;
    }

    if (top > 0xFFFFu)
    {
        top = 0xFFFFu;
    }

    /* Set period for overflow rate */
    TCA0.SINGLE.PER = (uint16_t)top;

    /* Enable timer */
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Configure EVSYS to route TCA0 overflow event to ADC0 start.
 *
 * Channel 0 generator: TCA0 overflow
 * User: ADC0 start (device-pack naming varies)
 */
static void evsys_route_tca0_ovf_to_adc0(void)
{
    /* Configure EVSYS Channel 0 generator = TCA0 overflow */
    EVSYS.CHANNEL0 = (uint8_t)(EVSYS_CHANNEL0_TCA0_OVF_LUNF_gc);

    /* Connect Channel 0 to ADC0 user */
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc; 
}

/**
 * @brief Initialize ADC0 to be triggered by EVSYS events (hardware triggered).
 *
 * Configuration:
 * - Reference: VDD
 * - Prescaler: DIV4 (adjust as needed)
 * - Input: AIN0
 * - Event start enable (STARTEI)
 * - Result-ready interrupt enabled
 */
static void adc0_init_event_triggered(void)
{
    /* Disable ADC before config */
    ADC0.CTRLA = 0;

    /* Select reference and prescaler */
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV4_gc;

    /* Select input channel (example: AIN0) */
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    /* Enable "start on event" */
#if defined(ADC_STARTEI_bm)
    ADC0.EVCTRL = ADC_STARTEI_bm;
#else
    /* Some packs may name this differently; search for "STARTEI" in headers */
    ADC0.EVCTRL = 0;
#endif

    /* Enable result-ready interrupt */
    ADC0.INTCTRL = ADC_RESRDY_bm;

    /* Clear pending flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    /* Enable ADC */
    ADC0.CTRLA = ADC_ENABLE_bm;
}

/**
 * @brief ADC0 result-ready ISR.
 *
 * Runs after every timer-triggered conversion.
 */
ISR(ADC0_RESRDY_vect)
{
    /* Read result */
    g_latest_adc_sample = ADC0.RES;

    /* Clear flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

int main(void)
{
    /* 1) Configure timer to generate periodic overflow events */
    tca0_init_overflow_event(ADC_SAMPLE_HZ);

    /* 2) Route timer overflow event -> ADC start via EVSYS */
    evsys_route_tca0_ovf_to_adc0();

    /* 3) Configure ADC to start conversion on incoming event */
    adc0_init_event_triggered();

    /* Enable global interrupts (ADC ISR) */
    sei();

    while (1)
    {
        /*
         * Main loop is free. ADC sampling runs at ADC_SAMPLE_HZ in hardware.
         * You can process g_latest_adc_sample here without blocking.
         */
        uint16_t s = g_latest_adc_sample;
        (void)s;
    }
}