/*
 * adc.h
 *
 * ADC driver for AVR128DA48 — public API.
 * All register-level access is confined to adc.c.
 * Callers must include only this header.
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Reference voltage selection
 * Maps to VREF_REFSEL_*_gc values written to VREF.ADC0REF.
 * ----------------------------------------------------------------------- */
typedef enum
{
    ADC_REF_1V024 = 0,   /* Internal 1.024 V  (VREF_REFSEL_1V024_gc) */
    ADC_REF_2V048,       /* Internal 2.048 V  (VREF_REFSEL_2V048_gc) */
    ADC_REF_4V096,       /* Internal 4.096 V  (VREF_REFSEL_4V096_gc) */
    ADC_REF_2V500,       /* Internal 2.500 V  (VREF_REFSEL_2V500_gc) */
    ADC_REF_VDD,         /* VDD               (VREF_REFSEL_VDD_gc)    */
    ADC_REF_VREFA,       /* External VREFA    (VREF_REFSEL_VREFA_gc)  */
} adc_ref_t;

/* -----------------------------------------------------------------------
 * Positive-input channel selection
 * Values match ADC_MUXPOS_*_gc fields.
 * ----------------------------------------------------------------------- */
typedef enum
{
    ADC_CH_AIN0        = 0x00,
    ADC_CH_AIN1        = 0x01,
    ADC_CH_AIN2        = 0x02,
    ADC_CH_AIN3        = 0x03,
    ADC_CH_AIN4        = 0x04,
    ADC_CH_AIN5        = 0x05,
    ADC_CH_AIN6        = 0x06,
    ADC_CH_AIN7        = 0x07,
    ADC_CH_AIN8        = 0x08,
    ADC_CH_AIN9        = 0x09,
    ADC_CH_AIN10       = 0x0A,
    ADC_CH_AIN11       = 0x0B,
    ADC_CH_AIN16       = 0x10,
    ADC_CH_AIN17       = 0x11,
    ADC_CH_AIN18       = 0x12,
    ADC_CH_AIN19       = 0x13,
    ADC_CH_AIN20       = 0x14,
    ADC_CH_AIN21       = 0x15,
    ADC_CH_GND         = 0x40,   /* ADC_MUXPOS_GND_gc        */
    ADC_CH_TEMPSENSE   = 0x42,   /* ADC_MUXPOS_TEMPSENSE_gc  */
    ADC_CH_DAC0        = 0x48,   /* ADC_MUXPOS_DAC0_gc       */
    ADC_CH_DACREF0     = 0x49,   /* ADC_MUXPOS_DACREF0_gc    */
    ADC_CH_DACREF1     = 0x4A,   /* ADC_MUXPOS_DACREF1_gc    */
    ADC_CH_DACREF2     = 0x4B,   /* ADC_MUXPOS_DACREF2_gc    */
} adc_channel_t;

/* -----------------------------------------------------------------------
 * Clock prescaler — ADC_PRESC_*_gc
 * ADC clock = CLK_PER / divisor.  Keep ADC clock 150 kHz - 1.5 MHz.
 * At F_CPU = 4 MHz default: DIV4 -> 1 MHz is a safe choice.
 * ----------------------------------------------------------------------- */
typedef enum
{
    ADC_PRESC_DIV2   = 0x00,   /* ADC_PRESC_DIV2_gc   */
    ADC_PRESC_DIV4   = 0x01,   /* ADC_PRESC_DIV4_gc   */
    ADC_PRESC_DIV8   = 0x02,   /* ADC_PRESC_DIV8_gc   */
    ADC_PRESC_DIV12  = 0x03,   /* ADC_PRESC_DIV12_gc  */
    ADC_PRESC_DIV16  = 0x04,   /* ADC_PRESC_DIV16_gc  */
    ADC_PRESC_DIV20  = 0x05,   /* ADC_PRESC_DIV20_gc  */
    ADC_PRESC_DIV24  = 0x06,   /* ADC_PRESC_DIV24_gc  */
    ADC_PRESC_DIV28  = 0x07,   /* ADC_PRESC_DIV28_gc  */
    ADC_PRESC_DIV32  = 0x08,   /* ADC_PRESC_DIV32_gc  */
    ADC_PRESC_DIV48  = 0x09,   /* ADC_PRESC_DIV48_gc  */
    ADC_PRESC_DIV64  = 0x0A,   /* ADC_PRESC_DIV64_gc  */
    ADC_PRESC_DIV96  = 0x0B,   /* ADC_PRESC_DIV96_gc  */
    ADC_PRESC_DIV128 = 0x0C,   /* ADC_PRESC_DIV128_gc */
    ADC_PRESC_DIV256 = 0x0D,   /* ADC_PRESC_DIV256_gc */
} adc_prescaler_t;

/* -----------------------------------------------------------------------
 * Accumulation (oversampling) — ADC_SAMPNUM_*_gc written to ADC0.CTRLB
 * ----------------------------------------------------------------------- */
typedef enum
{
    ADC_SAMPLES_1   = 0x00,   /* ADC_SAMPNUM_NONE_gc  - no accumulation */
    ADC_SAMPLES_2   = 0x01,   /* ADC_SAMPNUM_ACC2_gc                    */
    ADC_SAMPLES_4   = 0x02,   /* ADC_SAMPNUM_ACC4_gc                    */
    ADC_SAMPLES_8   = 0x03,   /* ADC_SAMPNUM_ACC8_gc                    */
    ADC_SAMPLES_16  = 0x04,   /* ADC_SAMPNUM_ACC16_gc                   */
    ADC_SAMPLES_32  = 0x05,   /* ADC_SAMPNUM_ACC32_gc                   */
    ADC_SAMPLES_64  = 0x06,   /* ADC_SAMPNUM_ACC64_gc                   */
    ADC_SAMPLES_128 = 0x07,   /* ADC_SAMPNUM_ACC128_gc                  */
} adc_samples_t;

/* -----------------------------------------------------------------------
 * Driver configuration - pass a filled struct to ADC_Init().
 * ----------------------------------------------------------------------- */
typedef struct
{
    adc_ref_t       reference;   /* Voltage reference source          */
    adc_channel_t   channel;     /* Initial positive-input channel    */
    adc_prescaler_t prescaler;   /* ADC clock divider                 */
    adc_samples_t   samples;     /* Accumulation count (1 = none)     */
} adc_config_t;

/* -----------------------------------------------------------------------
 * Public API - no register names appear outside adc.c
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise ADC0 with the supplied configuration.
 *         Configures VREF.ADC0REF, ADC0.CTRLA, ADC0.CTRLB, ADC0.CTRLC,
 *         and the initial MUXPOS channel.
 *         The analogue pin (if using an external channel) must have its
 *         digital input buffer disabled before calling this function.
 * @param  cfg  Pointer to a fully-initialised adc_config_t.
 */
void ADC_Init(const adc_config_t *cfg);

/**
 * @brief  Change the positive-input channel without re-initialising.
 * @param  ch  New channel selection.
 */
void ADC_SetChannel(adc_channel_t ch);

/**
 * @brief  Start a conversion on the current channel and block until done.
 * @return Raw 12-bit result (0 - 4095).
 *         If accumulation > 1 the hardware accumulator value is returned;
 *         divide by the sample count in the application if averaging is
 *         desired.
 */
uint16_t ADC_Read(void);

/**
 * @brief  Convenience wrapper: select channel, then perform a blocking read.
 * @param  ch  Channel to sample.
 * @return Raw 12-bit result (0 - 4095).
 */
uint16_t ADC_ReadChannel(adc_channel_t ch);

/**
 * @brief  Convert a raw 12-bit ADC result to millivolts.
 * @param  raw      Raw ADC value (0 - 4095).
 * @param  vref_mv  Reference voltage in millivolts (e.g. 3300 for VDD=3V3).
 * @return Corresponding voltage in millivolts.
 */
uint32_t ADC_ToMillivolts(uint16_t raw, uint32_t vref_mv);

/**
 * @brief  Disable ADC0 (clears ADC_ENABLE_bm) to save power.
 */
void ADC_Disable(void);

/**
 * @brief  Re-enable ADC0 after a call to ADC_Disable().
 */
void ADC_Enable(void);

#endif /* ADC_H_ */
