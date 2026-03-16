/**
 * @file main.c
 * @brief DAC calibration demo using ADC loopback measurement.
 *
 * Summary:
 * - Outputs two DAC codes (low/high).
 * - Measures them with ADC0 through a physical loopback wire.
 * - Computes a linear correction model (gain + offset).
 * - Provides DAC0_set_millivolts_calibrated() to output a target voltage in mV.
 *
 * Hardware:
 * - Wire DAC0 output pin -> ADC0 input pin (example: ADC0 AIN0).
 * - Verify actual pin mapping for DAC0 OUT on your AVR128DA48 board.
 *
 * Clock:
 * - Assumes default 24 MHz system clock (not critical for this demo).
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define VREF_MV            4096u
#define DAC_MAX_CODE       1023u
#define ADC_MAX_CODE       1023u

/* Choose calibration points away from rails to avoid output buffer nonlinearity */
#define CAL_DAC_CODE_LOW   100u
#define CAL_DAC_CODE_HIGH  900u

/* ADC sampling */
#define ADC_SAMPLES        64u

/* Simple crude delay loop (for settling) */
static void delay_cycles(volatile uint32_t n)
{
    while (n--)
    {
        __asm__ __volatile__("nop");
    }
}

/**
 * @brief Initialize VREF for DAC0 (4.096V).
 */
static void VREF_init(void)
{
    /* Set DAC reference to 4.096V */
    VREF.DAC0REF = VREF_REFSEL_4V096_gc;

    /*
     * ADC reference selection differs by device configuration and register set.
     * For a clean demo: select the same 4.096V reference for ADC if your part supports it.
     *
     * If your device uses a separate ADC reference register, set it here.
     * Otherwise, keep ADC default and calibration still works as relative correction.
     */
}

/**
 * @brief Initialize DAC0 with output enabled.
 */
static void DAC0_init(void)
{
    /* Enable DAC output buffer and DAC */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

/**
 * @brief Set raw DAC code (0..1023).
 */
static void DAC0_set_code(uint16_t code)
{
    if (code > DAC_MAX_CODE)
    {
        code = DAC_MAX_CODE;
    }
    DAC0.DATA = code;
}

/**
 * @brief Initialize ADC0 for single-ended conversions on a selected analog input.
 *
 * IMPORTANT:
 * - This example selects ADC0 MUXPOS = AIN0 (often PA0).
 * - You MUST adjust MUXPOS to match the ADC pin you physically wired from DAC output.
 */
static void ADC0_init(void)
{
    /* Enable ADC, prescaler for reasonable ADC clock */
    ADC0.CTRLA = ADC_ENABLE_bm;

    /* Select prescaler (adjust if needed) */
    ADC0.CTRLB = ADC_PRESC_DIV16_gc;

    /* Select VREF reference (device-specific defaults may apply) */
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc; /* If you can select 4.096V ref, do it */

    /* Single-ended input mode (default on many AVR Dx) */
    ADC0.CTRLD = 0;

    /* Select input channel: AIN0 as example */
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    /* Optional: enable accumulation/averaging in hardware (not used here) */
}

/**
 * @brief Read one ADC sample (10-bit right-adjusted).
 */
static uint16_t ADC0_read_once(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;

    /* Wait for result ready */
    while ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0)
    {
        /* wait */
    }

    /* Clear flag */
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    /* Read result */
    return (uint16_t)ADC0.RES;
}

/**
 * @brief Read averaged ADC result (simple software average).
 */
static uint16_t ADC0_read_avg(uint16_t samples)
{
    uint32_t sum = 0;
    uint16_t i;

    for (i = 0; i < samples; i++)
    {
        sum += ADC0_read_once();
    }

    return (uint16_t)(sum / samples);
}

/*
 * Calibration model:
 *
 * We want: desired_dac_code -> corrected_dac_code
 *
 * We measure actual system behavior by loopback:
 *  - We output DAC code L, measure ADC code adcL
 *  - We output DAC code H, measure ADC code adcH
 *
 * Since ADC is our measurement instrument, we build a mapping:
 *  ADC_measured = A * DAC_code + B
 *
 * Then for a desired ADC-equivalent target (from desired voltage), we invert:
 *  DAC_code = (ADC_target - B) / A
 *
 * Implemented with integer fixed-point arithmetic.
 */

typedef struct
{
    int32_t A_q16;  /* slope in Q16: ADC_code per DAC_code */
    int32_t B;      /* offset in ADC codes */
} dac_cal_t;

static dac_cal_t g_cal;

/**
 * @brief Compute calibration coefficients using two-point measurement.
 */
static void DAC0_calibrate_two_point(void)
{
    uint16_t adcL, adcH;
    int32_t d_adc, d_dac;

    /* Output low code and measure */
    DAC0_set_code(CAL_DAC_CODE_LOW);
    delay_cycles(50000); /* settle */
    adcL = ADC0_read_avg(ADC_SAMPLES);

    /* Output high code and measure */
    DAC0_set_code(CAL_DAC_CODE_HIGH);
    delay_cycles(50000); /* settle */
    adcH = ADC0_read_avg(ADC_SAMPLES);

    /* Compute slope A and offset B for: ADC = A * DAC + B */
    d_adc = (int32_t)adcH - (int32_t)adcL;
    d_dac = (int32_t)CAL_DAC_CODE_HIGH - (int32_t)CAL_DAC_CODE_LOW;

    if (d_dac == 0 || d_adc == 0)
    {
        /* Fallback to ideal mapping if something went wrong */
        g_cal.A_q16 = (int32_t)((1L << 16)); /* 1.0 */
        g_cal.B = 0;
        return;
    }

    /* A in Q16: (d_adc / d_dac) */
    g_cal.A_q16 = (d_adc << 16) / d_dac;

    /* B = adcL - A*DAC_L */
    g_cal.B = (int32_t)adcL - (int32_t)((g_cal.A_q16 * (int32_t)CAL_DAC_CODE_LOW) >> 16);
}

/**
 * @brief Convert millivolts to an "ideal" ADC code target based on VREF_MV.
 *
 * This assumes ADC and DAC share the same reference and scale.
 * If ADC uses a different reference, this is still useful as a relative control,
 * but the absolute mV meaning will be off.
 */
static uint16_t mv_to_code_10bit(uint16_t mv)
{
    uint32_t code;

    if (mv >= VREF_MV)
    {
        return ADC_MAX_CODE;
    }

    code = ((uint32_t)mv * (uint32_t)ADC_MAX_CODE) / (uint32_t)VREF_MV;
    return (uint16_t)code;
}

/**
 * @brief Set DAC output to target voltage (mV) using calibration correction.
 */
static void DAC0_set_millivolts_calibrated(uint16_t mv)
{
    int32_t adc_target;
    int32_t dac_code;
    int32_t numerator;

    adc_target = (int32_t)mv_to_code_10bit(mv);

    /*
     * Invert: DAC = (ADC_target - B) / A
     * A is Q16, so:
     * DAC = ((ADC_target - B) << 16) / A_q16
     */
    numerator = (adc_target - g_cal.B) << 16;

    if (g_cal.A_q16 == 0)
    {
        dac_code = 0;
    }
    else
    {
        dac_code = numerator / g_cal.A_q16;
    }

    if (dac_code < 0)
    {
        dac_code = 0;
    }
    if (dac_code > (int32_t)DAC_MAX_CODE)
    {
        dac_code = (int32_t)DAC_MAX_CODE;
    }

    DAC0_set_code((uint16_t)dac_code);
}

int main(void)
{
    VREF_init();
    DAC0_init();
    ADC0_init();

    /* Run calibration once at boot */
    DAC0_calibrate_two_point();

    /*
     * Demo:
     * - Output a few voltages with calibration applied.
     * - Observe with multimeter / scope on DAC output.
     */
    while (1)
    {
        DAC0_set_millivolts_calibrated(500);
        delay_cycles(400000);

        DAC0_set_millivolts_calibrated(1500);
        delay_cycles(400000);

        DAC0_set_millivolts_calibrated(2500);
        delay_cycles(400000);

        DAC0_set_millivolts_calibrated(3500);
        delay_cycles(400000);
    }
}