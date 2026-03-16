/**
 * @file main.c
 * @brief Generates a sine wave using DAC0 and TCB0 periodic interrupt.
 *
 * MCU: AVR128DA48
 * Clock: 24 MHz
 * DAC reference: 4.096V
 * Sine samples: 32
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#define SINE_SAMPLES 32

/* 32-point sine table (centered at 512, amplitude ~400) */
static const uint16_t sine_table[SINE_SAMPLES] =
{
    512, 590, 665, 736, 800, 856, 902, 937,
    960, 972, 972, 960, 937, 902, 856, 800,
    736, 665, 590, 512, 434, 359, 288, 224,
    168, 122,  87,  64,  52,  52,  64,  87
};

static volatile uint8_t sine_index = 0;

/**
 * @brief Initialize DAC0 with 4.096V internal reference.
 */
static void DAC0_init(void)
{
    /* Select 4.096V reference for DAC */
    VREF.DAC0REF = VREF_REFSEL_4V096_gc;

    /* Enable DAC output and DAC itself */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

/**
 * @brief Initialize TCB0 in periodic interrupt mode.
 *
 * Generates interrupt at approximately 1 kHz.
 */
static void TCB0_init(void)
{
    /* Set compare value for interrupt frequency */
    TCB0.CCMP = 12000; /* 12 MHz / 12000 = 1 kHz */

    /* Enable interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Periodic interrupt mode, clock = CLK_PER/2 */
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

/**
 * @brief TCB0 interrupt service routine.
 *
 * Outputs next sine sample to DAC.
 */
ISR(TCB0_INT_vect)
{
    /* Clear interrupt flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;

    /* Output next sine sample */
    DAC0.DATA = sine_table[sine_index];

    sine_index++;

    if (sine_index >= SINE_SAMPLES)
    {
        sine_index = 0;
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
        /* All waveform generation handled in ISR */
    }
}