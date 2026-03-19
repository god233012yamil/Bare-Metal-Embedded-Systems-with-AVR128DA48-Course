/**
 * @file    main.c
 * @brief   Low-Power Periodic Wake-Up System – ADC0 Result Ready Interrupt
 *
 * Target:    AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *	 The only wake-up sources are the pin change interrupt, TWI address match, and CCL (if filter and
 *   edge-detect are disabled).	
 *   The MCU sleeps in Power-Down mode and wakes every ~1 second.
 *   The final wake-up source is the ADC0 RESRDY (Result Ready) interrupt.
 *
 *   Two-stage wake sequence:
 *   ?????????????????????????????????????????????????????????????????????
 *   Stage 1 – RTC PIT (1 s) wakes the CPU via its own interrupt.
 *             The PIT ISR software-starts a single ADC conversion and
 *             immediately returns so the main loop calls sleep_cpu() again.
 *
 *   Stage 2 – ADC0 completes the conversion (RUNSTBY keeps it running
 *             while the CPU is asleep) and raises RESRDY, waking the CPU
 *             a second time.  The RESRDY ISR reads the result and toggles
 *             the LED.
 *
 *   Why EVSYS is NOT used for the ADC trigger:
 *   ?????????????????????????????????????????????????????????????????????
 *   The EVSYS on the AVR128DA48 only exposes RTC PIT outputs up to
 *   DIV8192 on CH0 (i.e. 32768/8192 = 4 Hz minimum, 0.25 s minimum
 *   period).  There is no EVSYS_CHANNEL0_RTC_PIT_DIV32768_gc symbol in
 *   ioavr128da48.h.  A software start in the PIT ISR is therefore the
 *   correct way to achieve an exact 1-second trigger.
 *
 *   Corrected symbols (verified against ioavr128da48.h):
 *   ?????????????????????????????????????????????????????????????????????
 *   WRONG (previous version)            CORRECT (this file)
 *   ----------------------------------  --------------------------------
 *   EVSYS_CHANNEL0_RTC_PIT_DIV32768_gc  removed; ADC started in PIT ISR
 *   EVSYS_USER_ADC0START_CHANNEL_0_gc   removed; EVSYS not used
 *   ADC_REFSEL_1024MV_gc                VREF.ADC0REF = VREF_REFSEL_1V024_gc
 *
 *   ADC configuration:
 *   ?????????????????????????????????????????????????????????????????????
 *   - Input    : Internal temperature sensor (MUXPOS = TEMPSENSE_gc)
 *   - Reference: VREF peripheral, 1.024 V  (VREF_REFSEL_1V024_gc)
 *   - Prescaler: DIV256  ->  ADC clock = 4 MHz / 256  ~= 15.6 kHz
 *   - RUNSTBY  : Set -- ADC finishes conversion while CPU is sleeping
 *   - Interrupt: RESRDY -- fires when the result register is valid
 *
 * Hardware:
 *   LED0  -> PC6 (active LOW)
 *   ADC   -> Internal temperature sensor (no external wiring needed)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* ------------------------------------------------------------------ */
/* Defines                                                             */
/* ------------------------------------------------------------------ */
#define LED_PIN     PIN6_bm     /* PC6 - LED0 on Curiosity Nano (active LOW) */

/* ================================================================== */
/* Prototypes                                                          */
/* ================================================================== */
static void LED_init(void);
static void VREF_init(void);
static void ADC0_init(void);
static void RTC_PIT_init(void);
static void SLEEP_init(void);

/* ================================================================== */
/* Interrupt Service Routines                                          */
/* ================================================================== */

/**
 * RTC PIT Interrupt - fires every 1 second.
 *
 * Clear the PIT flag, then software-start a single ADC conversion.
 * Return immediately so the main loop calls sleep_cpu() again.
 * The ADC will complete the conversion with RUNSTBY active while the
 * CPU is back in Power-Down sleep.
 */
ISR(RTC_PIT_vect)
{
    /* Clear PIT interrupt flag */
    RTC.PITINTFLAGS = RTC_PI_bm;

    /* Software-start one ADC conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * ADC0 Result Ready Interrupt - fires when the conversion is complete.
 *
 * Reading ADC0.RES clears the RESRDY flag on AVR-Dx devices.
 * Toggle the LED to signal the 1-second wake-up event.
 */
ISR(ADC0_RESRDY_vect)
{
    /* Read result -- clears the RESRDY interrupt flag */
    (void)ADC0.RES;

    /* Toggle LED (PC6, active LOW) */
    PORTC.OUTTGL = LED_PIN;
}

/* ================================================================== */
/* Peripheral initialisation functions                                 */
/* ================================================================== */

/**
 * @brief Configure PC6 as output for LED0 (active LOW, off initially).
 */
static void LED_init(void)
{
    PORTC.DIRSET = LED_PIN;
    PORTC.OUTSET = LED_PIN;     /* LED off (active LOW) */
}

/**
 * @brief Configure the VREF peripheral to supply 1.024 V to ADC0.
 *
 * On AVR-Dx the ADC reference voltage is NOT selected inside the ADC
 * registers -- it is controlled by the VREF peripheral.
 * VREF_REFSEL_1V024_gc selects the internal 1.024 V bandgap reference.
 *
 * Previously wrong symbol : ADC_REFSEL_1024MV_gc  (does not exist)
 * Correct symbol           : VREF_REFSEL_1V024_gc  written to VREF.ADC0REF
 */
static void VREF_init(void)
{
    VREF.ADC0REF = VREF_REFSEL_1V024_gc;
}

/**
 * @brief Configure ADC0 for software-triggered single conversions.
 *
 * - MUXPOS  : Internal temperature sensor
 * - CTRLB   : Prescaler DIV256 -> ~15.6 kHz ADC clock
 * - CTRLA   : Enable ADC + RUNSTBY (finish conversion during Power-Down)
 * - INTCTRL : RESRDY interrupt enabled
 *
 * STARTEI is NOT set -- conversions are triggered by writing
 * ADC_STCONV_bm to ADC0.COMMAND inside the PIT ISR.
 */
static void ADC0_init(void)
{
    /* Internal temperature sensor as positive input */
    ADC0.MUXPOS  = ADC_MUXPOS_TEMPSENSE_gc;

    /* Prescaler DIV256: 4 MHz / 256 ~= 15.6 kHz ADC clock */
    ADC0.CTRLB   = ADC_PRESC_DIV256_gc;

    /* Enable ADC; keep running in standby so conversion completes in sleep */
    ADC0.CTRLA   = ADC_ENABLE_bm
                 | ADC_RUNSTBY_bm;

    /* Enable Result Ready interrupt */
    ADC0.INTCTRL = ADC_RESRDY_bm;
}

/**
 * @brief Configure the RTC PIT for a 1-second periodic interrupt.
 *
 * Clock source : OSC32K (internal 32.768 kHz oscillator)
 * Period       : CYC32768 -> 32768 / 32768 Hz = 1.000 s
 *
 * The PIT interrupt wakes the CPU briefly to software-start the ADC.
 */
static void RTC_PIT_init(void)
{
    /* Select internal 32.768 kHz oscillator */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* Wait for PIT registers to be ready */
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm)
    {
        ;
    }

    /* Enable PIT with 1-second period */
    RTC.PITCTRLA   = RTC_PERIOD_CYC32768_gc
                   | RTC_PITEN_bm;

    /* Enable PIT CPU interrupt */
    RTC.PITINTCTRL = RTC_PI_bm;
}

/**
 * @brief Select Power-Down as the sleep mode.
 *
 * The RTC OSC32K domain keeps running in Power-Down.
 * ADC0 with RUNSTBY set can complete a conversion in Power-Down.
 * Both RTC_PIT_vect and ADC0_RESRDY_vect can wake the CPU.
 */
static void SLEEP_init(void)
{
    SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc  /* Power-Down */
                  | SLPCTRL_SEN_bm;
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */
int main(void)
{
	// Configure PC6 as output for LED0.
	LED_init();
	
	// Configure the VREF peripheral to supply 1.024 V to ADC0.
	VREF_init();
	
	// Configure ADC0 for software-triggered single conversions.
	ADC0_init();
	
	// Configure the RTC PIT for a 1-second periodic interrupt.
	RTC_PIT_init();
	
	// Select Power-Down as the sleep mode.
	SLEEP_init();

	// Enable Global interrupt
	sei();          

	for (;;)
	{
		// Power-Down; wake on PIT (starts ADC) or ADC RESRDY
		sleep_cpu(); 
	}
}