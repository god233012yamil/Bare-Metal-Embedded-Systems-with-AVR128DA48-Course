/*
 * AVR128DA48 - ADC Battery Voltage Measurement
 *
 * Use Case: Measure battery voltage every 1 ms
 * Design:
 *   - TCB0 generates a 1 kHz event (1 ms period)
 *   - EVSYS routes TCB0 event to ADC
 *   - ADC in single conversion mode triggered by event
 *   - ISR stores the ADC result
 *   - Main loop computes running average of 16 samples
 *
 * Hardware: AVR128DA48 Curiosity Nano
 * Clock:    4 MHz internal oscillator (default)
 *
 * Battery voltage measurement:
 *   Connect battery voltage (via resistor divider to keep within 0-VDD)
 *   to AIN0 (PD0). The divider ratio must be accounted for in the
 *   voltage calculation below.
 *
 *   Example divider: R1=10k (high side), R2=10k (low side)
 *   -> Vin_max = 2 * VDD = 2 * 3.3V = 6.6V max battery voltage
 *
 * Device Pack: AVR-Dx 2.4.286
 */

#define F_CPU 4000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------- */

/* Number of samples to average in the main loop */
#define ADC_AVG_SAMPLES     16u

/* Voltage reference: use VDD as reference (handy for battery measurement
 * relative to supply, or use internal 2.048 V for absolute measurement).
 * Set to 1 to use the internal 2.048 V reference instead of VDD.       */
#define USE_INTERNAL_VREF   0

/* ADC input channel: AIN0 = PD0                                         */
#define ADC_MUXPOS_CHANNEL  ADC_MUXPOS_AIN0_gc

/* Resistor divider ratio (battery_voltage = adc_voltage * DIVIDER_RATIO)
 * For equal R1=R2 the ratio is 2.0.                                     */
#define DIVIDER_RATIO       2.0f

/* VDD assumed voltage in millivolts (used if VDD reference is selected) */
#define VDD_MV              3300u

/* Internal VREF in millivolts                                           */
#define INTERNAL_VREF_MV    2048u

/* -----------------------------------------------------------------------
 * Shared variables between ISR and main loop
 * --------------------------------------------------------------------- */

/* Circular buffer for ADC results filled by ISR */
#define BUFFER_SIZE         16u

volatile uint16_t adc_buffer[BUFFER_SIZE];
volatile uint8_t  adc_write_idx = 0;
volatile uint8_t  adc_sample_count = 0;  /* counts up to BUFFER_SIZE    */
volatile bool     adc_new_data = false;

/* -----------------------------------------------------------------------
 * Function prototypes
 * --------------------------------------------------------------------- */
static void clock_init(void);
static void port_init(void);
static void vref_init(void);
static void adc_init(void);
static void tcb0_init(void);
static void evsys_init(void);
static uint32_t compute_average(void);
static uint32_t adc_to_mv(uint32_t adc_avg);

/* -----------------------------------------------------------------------
 * ISR: ADC Result Ready
 * --------------------------------------------------------------------- */
ISR(ADC0_RESRDY_vect)
{
    /* Read result to clear the interrupt flag (also clears RESRDY bit)  */
    uint16_t result = ADC0.RES;

    adc_buffer[adc_write_idx] = result;
    adc_write_idx = (adc_write_idx + 1u) & (BUFFER_SIZE - 1u);

    if (adc_sample_count < BUFFER_SIZE)
    {
        adc_sample_count++;
    }

    adc_new_data = true;
}

/* -----------------------------------------------------------------------
 * Main
 * --------------------------------------------------------------------- */
int main(void)
{
    clock_init();
    port_init();
    vref_init();
    adc_init();
    evsys_init();
    tcb0_init();

    sei();  /* Enable global interrupts */

    /* Status LED: PC6 on Curiosity Nano (active LOW)                    */
    PORTC.DIRSET = PIN6_bm;
    PORTC.OUTCLR = PIN6_bm;  /* LED on initially                        */

    uint32_t battery_mv = 0;
    uint8_t  avg_counter = 0;
    uint32_t accumulator = 0;

    while (1)
    {
        if (adc_new_data)
        {
            /* Safely read the flag and the latest sample index          */
            cli();
            adc_new_data = false;
            uint8_t read_idx = (uint8_t)((adc_write_idx == 0u)
                                          ? (BUFFER_SIZE - 1u)
                                          : (adc_write_idx - 1u));
            uint16_t sample = adc_buffer[read_idx];
            sei();

            /* Accumulate ADC_AVG_SAMPLES samples for a moving average   */
            accumulator += sample;
            avg_counter++;

            if (avg_counter >= ADC_AVG_SAMPLES)
            {
                uint32_t adc_avg = accumulator / ADC_AVG_SAMPLES;
                battery_mv = adc_to_mv(adc_avg);

                accumulator = 0;
                avg_counter = 0;

                /* Toggle LED every time a new averaged result is ready  */
                PORTC.OUTTGL = PIN6_bm;

                /*
                 * battery_mv now contains the calculated battery
                 * voltage in millivolts.
                 *
                 * Example: send over USART, display on LCD, etc.
                 * For this demo the value is held in the variable and
                 * the LED toggles as a heartbeat.
                 *
                 * To inspect the value:
                 *   - Use the Atmel Studio / Microchip Studio debugger
                 *     and add 'battery_mv' to the Watch window.
                 *   - Or set a breakpoint on the line below.
                 */
                (void)battery_mv;   /* suppress unused-variable warning  */
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * Clock initialisation
 * Use default 4 MHz internal oscillator – no changes needed.
 * --------------------------------------------------------------------- */
static void clock_init(void)
{
    /* AVR128DA48 boots with OSCHF at 4 MHz – nothing to do.             */
}

/* -----------------------------------------------------------------------
 * Port initialisation
 * PD0 (AIN0) must be configured as input with digital input buffer
 * disabled to reduce noise on the ADC input.
 * --------------------------------------------------------------------- */
static void port_init(void)
{
    /* Disable digital input buffer on PD0 (AIN0)                       */
    PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
}

/* -----------------------------------------------------------------------
 * Voltage Reference
 * --------------------------------------------------------------------- */
static void vref_init(void)
{
#if USE_INTERNAL_VREF
    /* Select 2.048 V internal reference for ADC0                       */
    VREF.ADC0REF = VREF_REFSEL_2V048_gc | VREF_ALWAYSON_bm;
#else
    /* VDD as reference – no VREF configuration needed, selected in ADC */
#endif
}

/* -----------------------------------------------------------------------
 * ADC initialisation
 * --------------------------------------------------------------------- */
static void adc_init(void)
{
    /* Select input channel                                              */
    ADC0.MUXPOS = ADC_MUXPOS_CHANNEL;

    /*
     * Clock prescaler: ADC clock must be 0.15 – 2 MHz (12-bit mode).
     * F_CPU = 4 MHz -> prescaler DIV4 -> ADC_CLK = 1 MHz.
     */
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;

    /* Delay: 32 ADC clock cycles between event and conversion start    */
    ADC0.CTRLD = ADC_INITDLY_DLY32_gc;

    /* Enable Result Ready interrupt                                     */
    ADC0.INTCTRL = ADC_RESRDY_bm;

    /*
     * EVACT: start conversion on event
     * FREERUN: disabled (single conversion per event)
     * Resolution: 12-bit
     * Enable ADC
     */
    ADC0.EVCTRL = ADC_STARTEI_bm;  /* Start on event input              */
    ADC0.CTRLA  = ADC_ENABLE_bm;   /* 12-bit resolution is default      */
}

/* -----------------------------------------------------------------------
 * TCB0 – 1 kHz periodic event generator
 *
 * TCB in Periodic Interrupt mode with CAPTEI output used as event source.
 * Period: F_CPU / prescaler / TOP = 4,000,000 / 2 / 2000 = 1000 Hz
 *
 * TCB clock prescaler options: DIV1 or DIV2 only on AVR-Dx.
 * Using DIV2: TCB_CLK = 2 MHz, TOP = 1999 => period = 1 ms exactly.
 * --------------------------------------------------------------------- */
static void tcb0_init(void)
{
    /* 1 ms period: CLK_PER/2 = 2 MHz, count to 2000 */
    TCB0.CCMP    = 1999u;           /* TOP value (0-based)               */
    TCB0.CTRLB   = TCB_CNTMODE_INT_gc; /* Periodic Interrupt mode        */
    TCB0.EVCTRL  = TCB_CAPTEI_bm;   /* Enable capture/compare event out  */
    /* Note: on AVR-Dx TCB generates event on compare match automatically
     * when CAPTEI output is enabled in INT mode.                        */
    TCB0.CTRLA   = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

/* -----------------------------------------------------------------------
 * EVSYS – route TCB0 capture event to ADC start-of-conversion
 * --------------------------------------------------------------------- */
static void evsys_init(void)
{
    /*
     * Event channel 0: generator = TCB0 capture event
     * User: ADC0 START
     *
     * AVR128DA48 EVSYS:
     *   CHANNEL0 generator = TCB0 CAPT
     *   USERADC0START      = CHANNEL0
     */
    EVSYS.CHANNEL0    = EVSYS_CHANNEL0_TCB0_CAPT_gc;
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;
}

/* -----------------------------------------------------------------------
 * Compute average of samples currently in the circular buffer
 * (used optionally; main loop uses a simpler running accumulator)
 * --------------------------------------------------------------------- */
static uint32_t compute_average(void)
{
    cli();
    uint8_t count = adc_sample_count;
    uint16_t buf_copy[BUFFER_SIZE];
    for (uint8_t i = 0; i < BUFFER_SIZE; i++)
    {
        buf_copy[i] = adc_buffer[i];
    }
    sei();

    if (count == 0u) return 0u;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        sum += buf_copy[i];
    }
    return sum / count;
}

/* -----------------------------------------------------------------------
 * Convert averaged ADC value to battery voltage in millivolts
 * --------------------------------------------------------------------- */
static uint32_t adc_to_mv(uint32_t adc_avg)
{
#if USE_INTERNAL_VREF
    uint32_t vref_mv = INTERNAL_VREF_MV;
#else
    uint32_t vref_mv = VDD_MV;
#endif

    /*
     * 12-bit ADC: full scale = 4095 counts = vref_mv
     * adc_voltage_mv = (adc_avg * vref_mv) / 4095
     * battery_mv     = adc_voltage_mv * DIVIDER_RATIO
     */
    uint32_t adc_voltage_mv = (adc_avg * vref_mv) / 4095u;
    uint32_t battery_mv     = (uint32_t)((float)adc_voltage_mv * DIVIDER_RATIO);
    return battery_mv;
}