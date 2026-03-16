/**
 * @file main.c
 * @brief Demonstrates basic DAC usage on AVR128DA48 to Generate 
 *        a simple staircase waveform using DAC.
 */
#include <avr/io.h>

/**
 * @brief Initialize DAC0 with internal 4.096V reference.
 */
static void DAC0_init(void)
{
    /* Select 4.096V reference for DAC */
    VREF.DAC0REF = VREF_REFSEL_4V096_gc;

    /* Enable DAC output buffer and DAC */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

/**
 * @brief Set DAC output value.
 * 
 * @param value 10-bit value (0–1023)
 */
static void DAC0_set(uint16_t value)
{
    DAC0.DATA = value;
}

/**
 * @brief Generate a simple staircase waveform using DAC.
 */
int main(void)
{
    uint16_t value = 0;

    DAC0_init();

    while (1)
    {
        DAC0_set(value);

        value += 32;

        if (value > 1023)
        {
            value = 0;
        }

        for (volatile uint32_t i = 0; i < 50000; i++)
        {
            /* crude delay */
        }
    }
}