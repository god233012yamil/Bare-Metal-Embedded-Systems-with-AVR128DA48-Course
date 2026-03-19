/**
 * @file    main.c
 * @brief   Low-Power Periodic Wake-Up System – RTC Overflow Interrupt
 *
 * Target:    AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *   The RTC is driven by the 32.768 kHz internal oscillator (OSC32K).
 *   The RTC counter counts from 0 up to the value loaded in the PERIOD
 *   register (PER).  When the counter overflows (CNT == PER) an OVF
 *   (Overflow) interrupt fires and the counter resets to 0.
 *
 *   Period calculation:
 *     Desired period = 1 s
 *     RTC clock      = 32 768 Hz  (OSC32K, prescaler = 1)
 *     PER value      = (RTC_clock × desired_period) - 1
 *                    = (32 768 × 1) - 1
 *                    = 32 767  (0x7FFF)
 *
 *   In the ISR the on-board LED (PC6, active-LOW on the Curiosity Nano)
 *   is toggled.  The CPU then returns to Power-Down sleep and stays there
 *   until the next OVF event.  Active time per second is only a handful
 *   of microseconds.
 *
 * Hardware:
 *   LED0  -> PC6  (active LOW – driven LOW to turn on)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* ------------------------------------------------------------------ */
/* Defines                                                             */
/* ------------------------------------------------------------------ */
#define LED_PIN         PIN6_bm     /* PC6 – LED0 on Curiosity Nano   */

/* RTC period for 1-second overflow:
 * PER = (32768 Hz * 1 s) - 1 = 32767 = 0x7FFF                       */
#define RTC_PERIOD_1S   0x7FFFu

/* ------------------------------------------------------------------ */
/* Prototypes                                                          */
/* ------------------------------------------------------------------ */
static void LED_init(void);
static void RTC_init(void);
static void SLEEP_init(void);

/* ================================================================== */
/* Interrupt Service Routine                                           */
/* ================================================================== */
/**
 * RTC Overflow Interrupt – fires every 1 second.
 * Toggle the LED, then return so the CPU goes back to sleep.
 */
ISR(RTC_CNT_vect)
{
    /* Clear the OVF interrupt flag */
    RTC.INTFLAGS = RTC_OVF_bm;

    /* Toggle LED (PC6, active LOW) */
    PORTC.OUTTGL = LED_PIN;
}

/* ================================================================== */
/* Peripheral initialisation functions                                 */
/* ================================================================== */

/**
 * @brief Configure PC6 as a push-pull output for LED0.
 *        LED is active LOW – drive HIGH (off) initially.
 */
static void LED_init(void)
{
    PORTC.DIRSET = LED_PIN;     /* PC6 as output          */
    PORTC.OUTSET = LED_PIN;     /* LED off initially      */
}

/**
 * @brief Configure the RTC for a 1-second overflow interrupt.
 *
 * Steps:
 *  1. Select the 32.768 kHz internal oscillator as RTC clock source.
 *  2. Wait for the CTRLA sync-busy flag to clear.
 *  3. Load the PERIOD register (PER = 32767 ? overflow at 32768 ticks = 1 s).
 *  4. Enable the OVF interrupt.
 *  5. Enable the RTC with prescaler = 1 (no prescaling).
 */
static void RTC_init(void)
{
    /* 1. Select internal 32.768 kHz oscillator */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* Wait for all register to be synchronized */
    while (RTC.STATUS > 0) { ; }

    /* 3. Set the period: OVF fires when CNT goes from PER back to 0.
     *    PER = 32767  ?  32768 RTC clocks = 1 second                 */
    RTC.PER = RTC_PERIOD_1S;

    /* 4. Enable OVF interrupt */
    RTC.INTCTRL = RTC_OVF_bm;

    /* Wait for all register to be synchronized */
    while (RTC.STATUS > 0)
    {
        ;
    }
    
	/* 5. Enable RTC, prescaler DIV1 (no prescaling), no correction    */
	RTC.CTRLA = RTC_PRESCALER_DIV1_gc  /* Prescaler = 1              */
              | RTC_RTCEN_bm            /* Enable RTC                 */
              | RTC_RUNSTDBY_bm;        /* Keep running in standby    */
}

/**
 * @brief Select Power-Down as the sleep mode.
 *        In Power-Down the CPU and most peripherals are stopped;
 *        the RTC continues running from its asynchronous clock domain.
 */
static void SLEEP_init(void)
{
    SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc  /* Power-Down mode */
                  | SLPCTRL_SEN_bm;          /* Sleep enable    */
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */
int main(void)
{
	// Configure PC6 as a push-pull output for LED0.
	LED_init();
	
	// Configure the RTC for a 1-second overflow interrupt.
	RTC_init();
	
	// Select Power-Down as the sleep mode.
	SLEEP_init();

	// Enable Global interrupt
	sei();          

	for (;;)
	{
		// Enter Power-Down; wake on RTC OVF interrupt
		sleep_cpu(); 
	}
}