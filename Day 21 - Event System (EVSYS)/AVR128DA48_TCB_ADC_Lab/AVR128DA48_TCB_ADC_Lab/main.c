/*
 * AVR128DA48 - TCB0 Periodic Event -> EVSYS Channel 0 -> ADC0 Auto-Trigger
 *
 * Architecture:
 *   TCB0 OVF event  -->  EVSYS Channel 0  -->  ADC0 start conversion
 *
 * The CPU never writes ADC_STCONV_bm.  Conversions are started automatically
 * by the Event System every time TCB0 overflows.  The CPU only reads results.
 *
 * Hardware:  AVR128DA48 Curiosity Nano
 *   PD2 / AIN2  =  ADC input  (connect a pot between VDD and GND)
 *   PC6 / LED0  =  active-low LED, toggles on every new ADC result
 *
 * Toolchain: Atmel Studio 7 / Microchip Studio
 *            AVR-Dx Device Pack 2.4.286
 *
 * --------------------------------------------------------------------------
 * Clock / timing
 * --------------------------------------------------------------------------
 * F_CPU        = 24 MHz  (AVR128DA48 default internal oscillator)
 * TCB0 clock   = CLK_PER / 2  = 12 MHz  (TCB_CLKSEL_DIV2_gc)
 * Sample rate  = f_TCB / (CCMP + 1) = 12 000 000 / 60 000 = 200 Hz
 * ADC clock    = CLK_PER / 12 = 2 MHz  (ADC_PRESC_DIV12_gc, within 0.5-6 MHz)
 * --------------------------------------------------------------------------
 */

#define F_CPU 24000000UL   /* AVR128DA48 default: internal 24 MHz oscillator */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* --------------------------------------------------------------------------
 * Timing constant
 *
 * TCB0 tick clock = CLK_PER / 2 = 12 MHz
 * CCMP = (f_tick / f_sample) - 1 = (12 000 000 / 200) - 1 = 59 999 = 0xEA5F
 * Must fit in uint16_t (max 65535) -> 59999 OK
 * -------------------------------------------------------------------------- */
#define TCB0_CCMP  ((uint16_t)0xEA5FU)   /* 200 Hz sample rate */

/* --------------------------------------------------------------------------
 * Globals  (written by ISR, read by main loop)
 * -------------------------------------------------------------------------- */
volatile uint16_t adc_result = 0;
volatile uint8_t  adc_new    = 0;

/* --------------------------------------------------------------------------
 * ADC0 Result-Ready ISR
 * Reading ADC0.RES clears the RESRDY flag automatically.
 * -------------------------------------------------------------------------- */
ISR(ADC0_RESRDY_vect)
{
    adc_result = ADC0.RES;
    adc_new    = 1;
}

/* --------------------------------------------------------------------------
 * gpio_init
 * -------------------------------------------------------------------------- */
static void gpio_init(void)
{
    /* LED0 = PC6, active-low output */
    PORTC.DIRSET   = PIN6_bm;
    PORTC.OUTSET   = PIN6_bm;                    /* LED off */

    /* PD2 = AIN2: disable digital input buffer to reduce noise */
    PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;  /* verified: header line 1782 */
}

/* --------------------------------------------------------------------------
 * tcb0_init  -  Periodic Interrupt mode, 200 Hz
 *
 * TCB0 counts CLK_PER/2 ticks.  When CNT reaches CCMP the counter resets
 * and an OVF event pulse is generated (one CLK_PER wide).
 * That pulse is the generator for EVSYS Channel 0.
 * No CPU interrupt from TCB0 is used.
 *
 * Symbols verified in ioavr128da48.h:
 *   TCB_CNTMODE_INT_gc   line 2462
 *   TCB_CLKSEL_DIV2_gc   line 2453  (was TCB_CLKSEL_CLKDIV1_gc - does not exist)
 *   TCB_ENABLE_bm        line 6243
 * -------------------------------------------------------------------------- */
static void tcb0_init(void)
{
    TCB0.CCMP    = TCB0_CCMP;
    TCB0.CTRLB   = TCB_CNTMODE_INT_gc;            /* Periodic Interrupt mode  */
    TCB0.EVCTRL  = 0;                             /* no event input to TCB0   */
    TCB0.INTCTRL = 0;                             /* no CPU interrupt needed  */
    TCB0.CNT     = 0;
    TCB0.CTRLA   = TCB_CLKSEL_DIV2_gc             /* f_tick = CLK_PER/2       */
                 | TCB_ENABLE_bm;
}

/* --------------------------------------------------------------------------
 * evsys_init  -  TCB0 OVF -> Channel 0 -> ADC0 START
 *
 * Generator: EVSYS_CHANNEL0_TCB0_OVF_gc  (line 897)
 *   OVF is the event emitted in Periodic Interrupt mode.
 *   CAPT (line 896) is for Input Capture modes - wrong for this use.
 *
 * User register: EVSYS.USERADC0START  (line 809 / 3259)
 * User value:    EVSYS_USER_CHANNEL0_gc  (line 1497)
 *   The compound symbol EVSYS_USER_ADC0START_CHANNEL0_gc does NOT exist.
 * -------------------------------------------------------------------------- */
static void evsys_init(void)
{
    EVSYS.CHANNEL0      = EVSYS_CHANNEL0_TCB0_OVF_gc;   /* generator */
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;        /* user      */
}

/* --------------------------------------------------------------------------
 * adc0_init  -  12-bit, event-triggered single conversions
 *
 * Register layout for AVR128DA48 (verified in ioavr128da48.h):
 *   ADC0.CTRLB  = Sample Accumulation Number  (ADC_SAMPNUM_t, line 337)
 *   ADC0.CTRLC  = Clock Prescaler             (ADC_PRESC_t,   line 290)
 *   There is NO REFSEL field in ADC0.CTRLC on this device.
 *   Reference voltage is set exclusively in the VREF peripheral.
 *
 * Reference setup (as specified):
 *   VREF.ADC0REF = (0 << VREF_ALWAYSON_bp) | VREF_REFSEL_2V048_gc
 *   VREF_ALWAYSON_bp  line 6862
 *   VREF_REFSEL_2V048_gc  line 2910
 *
 * ADC clock = CLK_PER / 12 = 24 MHz / 12 = 2 MHz  (spec: 0.5-6 MHz for 12-bit)
 *
 * ADC_STARTEI_bm (line 4258): each event pulse from EVSYS starts one
 * conversion.  ADC0.COMMAND = ADC_STCONV_bm is NEVER written.
 * -------------------------------------------------------------------------- */
static void adc0_init(void)
{
    /* 1. Reference: 2.048 V internal, ALWAYSON disabled (default off) */
    VREF.ADC0REF  = (0 << VREF_ALWAYSON_bp)   /* Always-on: disabled        */
                  | VREF_REFSEL_2V048_gc;       /* Internal 2.048 V reference */

    /* 2. Input channel: AIN2 = PD2 */
    ADC0.MUXPOS   = ADC_MUXPOS_AIN2_gc;

    /* 3. CTRLB = Sample Accumulation: NONE (single result, no oversampling) */
    ADC0.CTRLB    = ADC_SAMPNUM_NONE_gc;

    /* 4. CTRLC = Clock Prescaler: CLK_PER/12 -> 2 MHz ADC clock */
    ADC0.CTRLC    = ADC_PRESC_DIV12_gc;

    /* 5. Sample duration: 0 (minimum; increase for high-impedance sources) */
    ADC0.SAMPCTRL = 0;

    /* 6. Enable Result-Ready interrupt */
    ADC0.INTCTRL  = ADC_RESRDY_bm;
	
	// Enables the event input as trigger for starting
	// an ADC conversion. conversion starts on each EVSYS event pulse.
	ADC0.EVCTRL = ADC_STARTEI_bm;

    /* 7. Enable ADC: 12-bit, event-triggered start
     *    ADC_STCONV_bm is NEVER written anywhere in this project.         */
    ADC0.CTRLA    = ADC_ENABLE_bm
                  | ADC_RESSEL_12BIT_gc;
}

/* --------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------- */
int main(void)
{
    gpio_init();
    tcb0_init();
    evsys_init();
    adc0_init();
    sei();

    while (1)
    {
        if (adc_new)
        {
            adc_new = 0;

            /*
             * adc_result = latest 12-bit ADC sample (0 - 4095).
             *
             * With VREF = 2.048 V:
             *   millivolts = (uint32_t)adc_result * 2048UL / 4096UL
             *              = adc_result >> 1
             *
             * Add your application logic here, for example:
             *   - Transmit via USART0
             *   - Apply a digital filter
             *   - Update a PWM duty cycle
             */

            /* Toggle LED0 to confirm conversions are running */
            PORTC.OUTTGL = PIN6_bm;
        }

        /* CPU is free to sleep (IDLE) or do other work between events. */
    }
}