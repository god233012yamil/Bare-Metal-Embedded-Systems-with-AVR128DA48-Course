/**
 * @file    main.c
 * @brief   Low-Power Periodic Wake-Up System – RTC Compare Match (CMP) Interrupt
 *
 * Target:    AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *   The RTC is driven by the 32.768 kHz internal oscillator (OSC32K).
 *   The RTC counter (CNT) counts freely from 0 up to PER (0xFFFF by default,
 *   i.e. free-running 16-bit counter).  A Compare Match interrupt fires every
 *   time CNT == CMP.  After the match, CNT is NOT reset automatically — the
 *   counter continues counting.  To achieve a repeating 1-second interval the
 *   CMP register is advanced by 32768 each time inside the ISR.
 *
 *   Period calculation:
 *     RTC clock       = 32 768 Hz  (OSC32K, prescaler = 1)
 *     Ticks per second = 32 768
 *     CMP step        = 32 768  (0x8000)
 *
 *   On the first match:  CNT == 32768  (1 s after reset)
 *   On the second match: CNT == 65536 ? wraps ? next CMP = 32768 again, etc.
 *   The 16-bit arithmetic wrap is handled naturally by uint16_t addition.
 *
 *   In the ISR the on-board LED (PC6, active-LOW on the Curiosity Nano) is
 *   toggled and CMP is advanced.  The CPU then returns to Power-Down sleep
 *   and stays there until the next CMP event.
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
#define LED_PIN          PIN6_bm    /* PC6 – LED0 on Curiosity Nano    */

/* Number of RTC ticks between consecutive compare matches = 1 second  */
#define RTC_CMP_STEP     ((uint16_t)32768u)   /* 32 768 ticks @ 32 768 Hz = 1 s */

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
 * RTC Compare Match Interrupt – fires every 1 second.
 *
 * 1. Clear the CMP interrupt flag.
 * 2. Advance the CMP register by one step (32768 ticks) so the next
 *    match is another 1 second away.  16-bit wrap is intentional and
 *    correct – it re-synchronises with the free-running CNT naturally.
 * 3. Toggle the LED.
 */
ISR(RTC_CNT_vect)
{
    /* 1. Clear the CMP interrupt flag */
    RTC.INTFLAGS = RTC_CMP_bm;

    /* 2. Advance compare register by one period (16-bit wrap is fine) */
    RTC.CMP = (uint16_t)(RTC.CMP + RTC_CMP_STEP);

    /* 3. Toggle LED (PC6, active LOW) */
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
 * @brief Configure the RTC for a periodic 1-second Compare Match interrupt.
 *
 * Steps:
 *  1. Select the 32.768 kHz internal oscillator as clock source.
 *  2. Wait for sync-busy on CTRLA.
 *  3. Set PER to 0xFFFF so the counter runs freely (full 16-bit range).
 *  4. Load the first CMP value (one step = 32768 ticks from CNT = 0).
 *  5. Enable the CMP interrupt.
 *  6. Enable the RTC with prescaler DIV1 and RUNSTDBY.
 *
 * Note: CNT starts at 0 after reset.  The first match fires exactly
 * RTC_CMP_STEP ticks later, i.e. after 1 second.
 */
static void RTC_init(void)
{
    /* 1. Select internal 32.768 kHz oscillator */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    /* 2. Wait for all register to be synchronized */
    while (RTC.STATUS > 0)
    {
        ;
    }

    /* 3. Free-running counter: PER = 0xFFFF (full 16-bit range) */
    RTC.PER = 0xFFFFu;

    /* 4. First compare value: match after 32768 ticks = 1 second     */
    RTC.CMP = RTC_CMP_STEP;

    /* 5. Enable CMP interrupt */
    RTC.INTCTRL = RTC_CMP_bm;

    /* Wait for all register to be synchronized */
    while (RTC.STATUS > 0)
    {
        ;
    }
    
	/* 6. Enable RTC: prescaler = DIV1, keep running in standby/sleep  */
	RTC.CTRLA = RTC_PRESCALER_DIV1_gc  /* No prescaling               */
              | RTC_RTCEN_bm            /* Enable RTC counter          */
              | RTC_RUNSTDBY_bm;        /* Run in Power-Down / Standby */
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
	
	// Configure the RTC for a periodic 1-second Compare Match interrupt.
	RTC_init();
	
	// Select Power-Down as the sleep mode.
	SLEEP_init();

	// Enable Global interrupt
	sei();          

	for (;;)
	{
		// Enter Power-Down; wake on RTC CMP interrupt
		sleep_cpu(); 
	}
}