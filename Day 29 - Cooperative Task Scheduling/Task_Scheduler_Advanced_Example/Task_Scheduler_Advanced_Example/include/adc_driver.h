#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>

/*
 * ADC0 configuration:
 *   Input:      AIN0 / PD0 (potentiometer wiper)
 *   Reference:  VDD (VREF_ADC0REFSEL_VDD_gc)
 *   Resolution: 12-bit (right-aligned)
 *   Prescaler:  DIV16 -> 4 MHz / 16 = 250 kHz ADC clock
 */

void     adc_init(void);
uint16_t adc_read_blocking(void);

#endif /* ADC_DRIVER_H */
