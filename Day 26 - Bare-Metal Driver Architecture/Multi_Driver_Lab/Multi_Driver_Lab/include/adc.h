/*
 * adc.h  --  ADC driver public API, AVR128DA48
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

typedef enum {
    ADC_REF_1V024 = 0,
    ADC_REF_2V048,
    ADC_REF_4V096,
    ADC_REF_2V500,
    ADC_REF_VDD,
    ADC_REF_VREFA,
} adc_ref_t;

typedef enum {
    ADC_CH_AIN0      = 0x00,
    ADC_CH_AIN1      = 0x01,
    ADC_CH_AIN2      = 0x02,
    ADC_CH_AIN3      = 0x03,
    ADC_CH_AIN4      = 0x04,
    ADC_CH_AIN5      = 0x05,
    ADC_CH_AIN6      = 0x06,
    ADC_CH_AIN7      = 0x07,
    ADC_CH_AIN8      = 0x08,
    ADC_CH_AIN9      = 0x09,
    ADC_CH_AIN10     = 0x0A,
    ADC_CH_AIN11     = 0x0B,
    ADC_CH_AIN16     = 0x10,
    ADC_CH_AIN17     = 0x11,
    ADC_CH_AIN18     = 0x12,
    ADC_CH_AIN19     = 0x13,
    ADC_CH_AIN20     = 0x14,
    ADC_CH_AIN21     = 0x15,
    ADC_CH_GND       = 0x40,  /* ADC_MUXPOS_GND_gc       */
    ADC_CH_TEMPSENSE = 0x42,  /* ADC_MUXPOS_TEMPSENSE_gc */
    ADC_CH_DAC0      = 0x48,  /* ADC_MUXPOS_DAC0_gc      */
    ADC_CH_DACREF0   = 0x49,  /* ADC_MUXPOS_DACREF0_gc   */
    ADC_CH_DACREF1   = 0x4A,  /* ADC_MUXPOS_DACREF1_gc   */
    ADC_CH_DACREF2   = 0x4B,  /* ADC_MUXPOS_DACREF2_gc   */
} adc_channel_t;

typedef enum {
    ADC_PRESC_DIV2   = 0x00,  /* ADC_PRESC_DIV2_gc   */
    ADC_PRESC_DIV4   = 0x01,  /* ADC_PRESC_DIV4_gc   */
    ADC_PRESC_DIV8   = 0x02,  /* ADC_PRESC_DIV8_gc   */
    ADC_PRESC_DIV12  = 0x03,  /* ADC_PRESC_DIV12_gc  */
    ADC_PRESC_DIV16  = 0x04,  /* ADC_PRESC_DIV16_gc  */
    ADC_PRESC_DIV20  = 0x05,  /* ADC_PRESC_DIV20_gc  */
    ADC_PRESC_DIV24  = 0x06,  /* ADC_PRESC_DIV24_gc  */
    ADC_PRESC_DIV28  = 0x07,  /* ADC_PRESC_DIV28_gc  */
    ADC_PRESC_DIV32  = 0x08,  /* ADC_PRESC_DIV32_gc  */
    ADC_PRESC_DIV48  = 0x09,  /* ADC_PRESC_DIV48_gc  */
    ADC_PRESC_DIV64  = 0x0A,  /* ADC_PRESC_DIV64_gc  */
    ADC_PRESC_DIV96  = 0x0B,  /* ADC_PRESC_DIV96_gc  */
    ADC_PRESC_DIV128 = 0x0C,  /* ADC_PRESC_DIV128_gc */
    ADC_PRESC_DIV256 = 0x0D,  /* ADC_PRESC_DIV256_gc */
} adc_prescaler_t;

typedef enum {
    ADC_SAMPLES_1   = 0x00,  /* ADC_SAMPNUM_NONE_gc  */
    ADC_SAMPLES_2   = 0x01,  /* ADC_SAMPNUM_ACC2_gc  */
    ADC_SAMPLES_4   = 0x02,  /* ADC_SAMPNUM_ACC4_gc  */
    ADC_SAMPLES_8   = 0x03,  /* ADC_SAMPNUM_ACC8_gc  */
    ADC_SAMPLES_16  = 0x04,  /* ADC_SAMPNUM_ACC16_gc */
    ADC_SAMPLES_32  = 0x05,  /* ADC_SAMPNUM_ACC32_gc */
    ADC_SAMPLES_64  = 0x06,  /* ADC_SAMPNUM_ACC64_gc */
    ADC_SAMPLES_128 = 0x07,  /* ADC_SAMPNUM_ACC128_gc*/
} adc_samples_t;

typedef struct {
    adc_ref_t       reference;
    adc_channel_t   channel;
    adc_prescaler_t prescaler;
    adc_samples_t   samples;
} adc_config_t;

void     ADC_Init(const adc_config_t *cfg);
void     ADC_SetChannel(adc_channel_t ch);
uint16_t ADC_Read(void);
uint16_t ADC_ReadChannel(adc_channel_t ch);
uint32_t ADC_ToMillivolts(uint16_t raw, uint32_t vref_mv);
void     ADC_Disable(void);
void     ADC_Enable(void);

#endif /* ADC_H_ */
