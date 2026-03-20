/*
 * Lab: Dual Interrupts - RTC (1s) + ADC Conversion Complete
 * Target : AVR128DA48 Curiosity Nano
 * Toolchain: Atmel/Microchip Studio 7, GCC, AVR-Dx Pack 2.4.286
 *
 * Pin assignments (Curiosity Nano):
 *   LED0  = PC6, active-LOW  (drive LOW = LED on, HIGH = LED off)
 *   SW0   = PC7, active-LOW  (not used in this lab, but noted)
 *   ADC   = AIN0 = PD0       (internal voltage reference / open pin)
 *
 * Interrupt sources:
 *   1. RTC PIT (Periodic Interrupt Timer) - 1 second period
 *   2. ADC0 RESRDY - result-ready after each conversion
 *
 * Symbol corrections vs. original (verified against ioavr128da48.h):
 *   RTC_SYNCBUSY_bm      -> RTC_CTRLABUSY_bm    (RTC.STATUS sync-busy flag)
 *   RTC_CLKSEL_INT32K_gc -> RTC_CLKSEL_OSC32K_gc (32.768 kHz from OSC32K)
 *   F_CPU 4000000UL      -> 20000000UL           (default OSCHF on AVR128DA48)
 *   ADC_PRESC_DIV32_gc   -> ADC_PRESC_DIV20_gc   (20 MHz / 20 = 1 MHz, in spec)
 */

#define F_CPU 20000000UL   /* AVR128DA48 default: OSCHF runs at 20 MHz */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* -- LED helper macros -------------------------------------------------- */
#define LED_PORT        PORTC
#define LED_PIN_bm      PIN6_bm          /* PC6 = LED0 on Curiosity Nano  */
#define LED_TOGGLE()    (LED_PORT.OUTTGL = LED_PIN_bm)
#define LED_OFF()       (LED_PORT.OUTSET = LED_PIN_bm)  /* active-LOW     */
#define LED_ON()        (LED_PORT.OUTCLR = LED_PIN_bm)

/* -- Shared data (ISR -> main) ------------------------------------------ */
volatile uint16_t adc_result = 0;       /* latest ADC sample              */
volatile uint8_t  adc_flag   = 0;       /* set by ISR, cleared by main    */


/* ========================================================================
 * RTC - Periodic Interrupt Timer (PIT)
 *
 * Clock source : OSC32K  (internal 32.768 kHz ultra-low-power oscillator)
 *                Enum:  RTC_CLKSEL_OSC32K_gc  (0x00, from ioavr128da48.h)
 * PIT period   : RTC_PERIOD_CYC32768_gc  ->  32768 / 32768 Hz  =  1.000 s
 *
 * Sync-busy flags (from ioavr128da48.h):
 *   RTC.STATUS    -> RTC_CTRLABUSY_bm  (RTC CTRLA sync busy)
 *   RTC.PITSTATUS -> RTC_CTRLBUSY_bm  (PIT CTRLA sync busy)
 * ======================================================================== */
static void RTC_init(void)
{
    /* 1. Wait until RTC CTRLA is not busy syncing to the async RTC clock.
     *    Correct flag: RTC_CTRLABUSY_bm  in  RTC.STATUS
     *    (RTC_SYNCBUSY_bm does not exist in AVR-Dx pack 2.4.286)          */
    while (RTC.STATUS & RTC_CTRLABUSY_bm);

    /* 2. Select the internal 32.768 kHz oscillator as the RTC clock source.
     *    Correct enum: RTC_CLKSEL_OSC32K_gc  (value 0x00)
     *    (RTC_CLKSEL_INT32K_gc does not exist in AVR-Dx pack 2.4.286)     */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* 3. Configure the PIT:
     *    Wait for PIT CTRLA sync (RTC_CTRLBUSY_bm in RTC.PITSTATUS)
     *    PERIOD = CYC32768 -> 32768 RTC cycles / 32768 Hz = 1 second
     *    ENABLE = 1        -> start the PIT                               */
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
    RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc   /* 1-second period             */
                 | RTC_PITEN_bm;            /* enable PIT                  */

    /* 4. Enable the PIT interrupt */
    RTC.PITINTCTRL = RTC_PI_bm;
}

/* RTC PIT ISR - fires every 1 second */
ISR(RTC_PIT_vect)
{
    RTC.PITINTFLAGS = RTC_PI_bm;    /* clear the interrupt flag (required!) */
    LED_TOGGLE();                   /* toggle LED0 on PC6                   */
}


/* ========================================================================
 * ADC0 - Single-ended, free-running, result-ready interrupt
 *
 * Input   : AIN0 / PD0  (floating -> reading will wander, which is fine
 *           for demonstrating the interrupt mechanism)
 * Vref    : internal 2.048 V  (INTREF)
 * CLK_ADC : F_CPU / 20 = 20 MHz / 20 = 1.000 MHz
 *           Datasheet spec: CLK_ADC must be 50 kHz - 2 MHz (12-bit mode)
 *           Prescaler enum: ADC_PRESC_DIV20_gc  (from ioavr128da48.h)
 * ======================================================================== */
static void ADC0_init(void)
{
    /* 1. Configure PD0 as input, disable digital input buffer to save power */
    PORTD.DIRCLR    = PIN0_bm;
    PORTD.PIN0CTRL  = PORT_ISC_INPUT_DISABLE_gc;

    /* 2. Set voltage reference for ADC to internal 2.048 V */
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;

    /* 3. Clock prescaler: DIV20 -> 20 MHz / 20 = 1.000 MHz (within spec)
     *    (DIV32 was used when F_CPU was 4 MHz -> 125 kHz, also in spec,
     *     but at 20 MHz DIV32 gives 625 kHz; DIV20 gives a cleaner 1 MHz) */
    ADC0.CTRLC = ADC_PRESC_DIV20_gc;

    /* 4. Select AIN0 (PD0) as the positive input                          */
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    /* 5. Enable result-ready interrupt                                     */
    ADC0.INTCTRL = ADC_RESRDY_bm;

    /* 6. Enable ADC in free-running mode, 12-bit resolution (default)     */
    ADC0.CTRLA = ADC_ENABLE_bm
               | ADC_FREERUN_bm;

    /* 7. Kick off the first conversion; free-run continues automatically  */
    ADC0.COMMAND = ADC_STCONV_bm;
}

/* ADC0 result-ready ISR - fires after every conversion
 * At 1 MHz ADC clock, 12-bit = ~33 cycles/conversion -> ~30,000 ISR/sec
 * Reading ADC0.RES automatically clears the RESRDY interrupt flag.       */
ISR(ADC0_RESRDY_vect)
{
    /* Reading RES automatically clears the RESRDY interrupt flag.           */
    adc_result = ADC0.RES;     /* 12-bit result, right-adjusted (default)    */
    adc_flag   = 1;            /* notify main loop                           */
}


/* ========================================================================
 * main
 * ======================================================================== */
int main(void)
{
    /* -- GPIO: configure LED pin as output, start with LED off -- */
    LED_PORT.DIRSET = LED_PIN_bm;
    LED_OFF();

    /* -- Peripheral init -- */
    RTC_init();
    ADC0_init();

    /* -- Enable global interrupts -- */
    sei();

    /* -- Main loop: mostly idle -- */
    while (1)
    {
        /*
         * The CPU sits here almost all the time.
         * Optionally use SLEEP / IDLE mode to save power - but for this
         * lab we simply check the ADC flag as a demonstration.
         */
        if (adc_flag)
        {
            adc_flag = 0;
            /*
             * adc_result now holds the latest 12-bit ADC sample.
             * You could transmit it via USART, display it, etc.
             * For this lab we just consume the flag to prevent it
             * from accumulating; the ISR keeps sampling in the background.
             */
            (void)adc_result;   /* suppress "unused variable" warning */
        }
    }
}