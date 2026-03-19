/**
 * @file    main.c
 * @brief   Low-Power Periodic Wake-Up System
 *
 * Target:  AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *   The RTC is driven by the 32.768 kHz internal oscillator (OSC32K).
 *   A Periodic Interrupt Timer (PIT) fires every 32 768 RTC clocks = 1 second.
 *   In the ISR the on-board LED (PC6, active-LOW on the Curiosity Nano) is
 *   toggled.  The CPU then immediately returns to Power-Down sleep and stays
 *   there until the next PIT event.  Active time per second is only a handful
 *   of microseconds.
 *
 * Hardware:
 *   LED0  -> PC6  (active LOW – driven LOW to turn on)
 *
 * RTC / PIT configuration:
 *   Clock source : Internal 32.768 kHz oscillator  (CLKSEL = OSC32K)
 *   PIT period   : RTC_PERIOD_CYC32768_gc  (32 768 / 32 768 Hz = 1 s)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* ------------------------------------------------------------------ */
/* Board-specific definitions                                          */
/* ------------------------------------------------------------------ */
#define LED_PIN     PIN6_bm   /* PC6 – LED0 on AVR128DA48 Curiosity Nano */

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
 * RTC Periodic Interrupt (fires every 1 second).
 * Toggle the LED, then return – the CPU goes straight back to sleep
 * in the main loop.
 */
ISR(RTC_PIT_vect)
{
    /* Clear the PIT interrupt flag */
    RTC.PITINTFLAGS = RTC_PI_bm;

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
    PORTC.DIRSET  = LED_PIN;   /* Set PC6 as output          */
    PORTC.OUTSET  = LED_PIN;   /* LED off initially (active LOW) */
}

/**
 * @brief Configure the RTC Periodic Interrupt Timer for a 1-second period.
 *
 * Steps:
 *  1. Select the 32.768 kHz internal oscillator as RTC clock.
 *  2. Wait until the clock source is ready.
 *  3. Enable the PIT with a 32 768-cycle period (= 1 s).
 *  4. Enable the PIT interrupt.
 */
static void RTC_init(void)
{
    /* 1. Select internal 32.768 kHz oscillator */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* 2. Wait for all register to be synchronized  */
    while (RTC.STATUS > 0)
    {
        ;
    }

    /* 3. Configure PIT: enable, period = 32 768 RTC cycles = 1 second */
    RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc   /* 1-second period            */
                 | RTC_PITEN_bm;             /* Enable PIT                 */

    /* 4. Enable PIT interrupt */
    RTC.PITINTCTRL = RTC_PI_bm;
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
	
	// Configure the RTC Periodic Interrupt Timer
	RTC_init();
	
	// Select Power-Down as the sleep mode.
	SLEEP_init();

	// Enable Global interrupt
	sei();          

	for (;;)
	{
		// Enter Power-Down; wake on PIT interrupt
		sleep_cpu(); 
	}
}
