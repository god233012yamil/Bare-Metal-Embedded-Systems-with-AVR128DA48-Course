/**
 * @file main.c
 * @brief Demonstrates basic DAC usage on AVR128DA48 to generate a fixed analog voltage.
 *
 * Configures the DAC to use the internal 4.096V reference and outputs ~2.0V.
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

int main(void)
{
    DAC0_init();

    /* Output ~2.0V */
    DAC0_set(500);

    while (1)
    {
        /* Steady output */
    }
}