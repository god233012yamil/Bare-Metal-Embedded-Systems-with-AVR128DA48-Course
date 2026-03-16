/**
 * @file main.c
 * @brief Generate a 1 Hz triangle wave using DAC0 and TCB0 interrupt.
 *
 * MCU: AVR128DA48
 * Clock: 24 MHz
 * DAC reference: 4.096V
 * Waveform: 1 Hz triangle
 */

 /*
 Day 19 - Lab Example
Generating a 1 Hz Triangle Wave Using DAC + TCB Interrupt
AVR128DA48 - Register-Level Only

This example generates a 1 Hz triangle wave on the DAC output using:
- DAC0 with internal 4.096V reference
- TCB0 in periodic interrupt mode
- Software ramp (no lookup table)
- Fully interrupt-driven
No blocking loops. No libraries. Just registers.

Design Strategy
We generate a triangle wave by:
1. Updating DAC value at a fixed rate.
2. Incrementing DAC code until max.
3. Decrementing until min.
4. Repeat.

Triangle frequency is determined by:

F_triangle = F_update / (2 * N_steps)

We choose:
N_steps = 512 steps
Update rate = 1024 Hz

So:
F_triangle = 1024 / (2 * 512) = 1 Hz

Timer Configuration
Assumptions:

F_CPU = 24 MHz

TCB0 clock = CLK_PER / 2 = 12 MHz

To get 1024 Hz interrupt:

12,000,000 / 1024 ≈ 11719

So:

TCB0.CCMP = 11719
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define DAC_MAX_CODE      1023u
#define TRIANGLE_STEPS    512u
#define TCB_CCMP_VALUE    11719u   /* 1024 Hz interrupt */

static volatile uint16_t dac_value = 0;
static volatile int8_t direction = 1;

/**
 * @brief Initialize DAC0 with internal 4.096V reference.
 */
static void DAC0_init(void)
{
    /* Select 4.096V reference */
    VREF.DAC0REF = VREF_REFSEL_4V096_gc;

    /* Enable DAC and output buffer */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

/**
 * @brief Initialize TCB0 in periodic interrupt mode.
 *
 * Generates 1024 Hz interrupt.
 */
static void TCB0_init(void)
{
    /* Set compare value */
    TCB0.CCMP = TCB_CCMP_VALUE;

    /* Enable interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Clock = CLK_PER/2, enable timer */
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

/**
 * @brief TCB0 interrupt service routine.
 *
 * Updates DAC value to generate triangle waveform.
 */
ISR(TCB0_INT_vect)
{
    /* Clear interrupt flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;

    /* Update DAC value */
    DAC0.DATA = dac_value;

    if (direction > 0)
    {
        dac_value += (DAC_MAX_CODE / TRIANGLE_STEPS);

        if (dac_value >= DAC_MAX_CODE)
        {
            dac_value = DAC_MAX_CODE;
            direction = -1;
        }
    }
    else
    {
        if (dac_value > (DAC_MAX_CODE / TRIANGLE_STEPS))
        {
            dac_value -= (DAC_MAX_CODE / TRIANGLE_STEPS);
        }
        else
        {
            dac_value = 0;
            direction = 1;
        }
    }
}

/**
 * @brief Main entry point.
 */
int main(void)
{
    DAC0_init();
    
    TCB0_init();

    /* Enable global interrupts */
    sei();

    while (1)
    {
        /* Waveform generation handled in ISR */
    }
}