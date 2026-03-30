/**
 * @file main.c
 * @brief Cooperative task scheduling example using TCB0 time base.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define LED_PORT   PORTC
#define LED_PIN    PIN6_bm

// Scheduler Flags
volatile uint8_t task_10ms_flag = 0u;
volatile uint8_t task_100ms_flag = 0u;
volatile uint8_t task_1000ms_flag = 0u;

volatile uint16_t g_adc_value = 0u;

typedef enum
{
    STATUS_STATE_IDLE = 0,
    STATUS_STATE_SAMPLE,
    STATUS_STATE_UPDATE
} status_state_t;

static status_state_t g_status_state = STATUS_STATE_IDLE;

/**
 * @brief Initialize LED pin.
 */
static void LED_init(void)
{
    LED_PORT.DIRSET = LED_PIN;
    LED_PORT.OUTCLR = LED_PIN;
}

/**
 * @brief Toggle LED output.
 */
static void LED_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN;
}

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
 * @brief Start ADC conversion.
 */
static void ADC0_start(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief Check whether ADC result is ready.
 *
 * @return 1 if ready, otherwise 0.
 */
static uint8_t ADC0_ready(void)
{
    return (ADC0.INTFLAGS & ADC_RESRDY_bm) ? 1u : 0u;
}

/**
 * @brief Read ADC result.
 *
 * @return ADC conversion result.
 */
static uint16_t ADC0_read(void)
{
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    return ADC0.RES;
}

/**
 * @brief Initialize TCB0 to generate a 1 ms scheduler tick.
 */
static void TCB0_init(void)
{
    // Disable TCB0 before configuration 
	TCB0.CTRLA &= ~(1 << TCB_ENABLE_bp);

	// Periodic interrupt mode 
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;

	// Set compare value for 1 ms interval
    // 24 MHz / 2 = 12 MHz, 12 MHz / 12000 = 1 kHz (1 ms period)
	TCB0.CCMP = 12000; 
	// Clear any pending interrupt flags 
	TCB0.INTFLAGS = TCB_CAPT_bm;

	// Enable capture/compare interrupt
	TCB0.INTCTRL = TCB_CAPT_bm;

	// Enable timer, use CLK_PER divided by 2 for better resolution
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

/**
 * @brief Task executed every 10 ms.
 *
 * Starts a new ADC conversion if previous one is complete.
 */
static void task_10ms(void)
{
    if (ADC0_ready())
    {
        g_adc_value = ADC0_read();
    }

    ADC0_start();
}

/**
 * @brief Task executed every 100 ms.
 *
 * Toggles LED.
 */
static void task_100ms(void)
{
    LED_toggle();
}

/**
 * @brief Task executed every 1000 ms.
 *
 * Demonstrates a simple cooperative state machine.
 */
static void task_1000ms(void)
{
    switch (g_status_state)
    {
        case STATUS_STATE_IDLE:
            g_status_state = STATUS_STATE_SAMPLE;
            break;

        case STATUS_STATE_SAMPLE:
            /* Use g_adc_value here if needed */
            g_status_state = STATUS_STATE_UPDATE;
            break;

        case STATUS_STATE_UPDATE:
            /* Update application status here */
            g_status_state = STATUS_STATE_IDLE;
            break;

        default:
            g_status_state = STATUS_STATE_IDLE;
            break;
    }
}

/**
 * @brief TCB0 ISR generates scheduler flags.
 */
ISR(TCB0_INT_vect)
{
    static uint16_t tick_count = 0u;

    TCB0.INTFLAGS = TCB_CAPT_bm;

    tick_count++;

    if ((tick_count % 10u) == 0u)
    {
        task_10ms_flag = 1u;
    }

    if ((tick_count % 100u) == 0u)
    {
        task_100ms_flag = 1u;
    }

    if ((tick_count % 1000u) == 0u)
    {
        task_1000ms_flag = 1u;
        tick_count = 0u;
    }
}

/**
 * @brief Main entry point.
 *
 * @return Never returns.
 */
int main(void)
{
    // Initialize LED pin
    LED_init();
    
    // Initialize ADC0 for periodic sampling
    ADC0_init();
    
    // Initialize TCB0 for scheduler tick generation
    TCB0_init();

    // Enable global interrupts
    sei();

    while (1)
    {
        if (task_10ms_flag)
        {
            task_10ms_flag = 0u;
            task_10ms();
        }

        if (task_100ms_flag)
        {
            task_100ms_flag = 0u;
            task_100ms();
        }

        if (task_1000ms_flag)
        {
            task_1000ms_flag = 0u;
            task_1000ms();
        }
    }
}