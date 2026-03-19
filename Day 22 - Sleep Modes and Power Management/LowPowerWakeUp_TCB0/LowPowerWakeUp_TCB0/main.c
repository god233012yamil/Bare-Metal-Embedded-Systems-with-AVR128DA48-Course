/**
 * @file    main.c
 * @brief   Low-Power Periodic Wake-Up System – TCB0 Periodic Interrupt (INT)
 *
 * Target:    AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *   TCB0 is configured in Periodic Interrupt mode (CNTMODE = INT).
 *   In this mode the counter counts from 0 up to CCMP, fires an interrupt,
 *   resets to 0, and repeats automatically — no manual register update needed.
 *
 *   Clock source and period calculation:
 *   ?????????????????????????????????????
 *   The main clock on the AVR128DA48 defaults to the internal 4 MHz oscillator
 *   (OSCHF at 4 MHz after reset, no PLL).  TCB0 is clocked from CLK_PER
 *   (= main clock) through its own prescaler.
 *
 *   Available TCB prescalers: DIV1, DIV2.
 *   For a 1-second period with a 16-bit counter (max 65535):
 *
 *     CLK_PER = 4 000 000 Hz
 *     Prescaler DIV2  ?  TCB clock = 2 000 000 Hz
 *     CCMP = (TCB_clock × period) - 1
 *          = (2 000 000 × 1) - 1
 *          = 1 999 999  ? EXCEEDS 16-bit range (max 65535)
 *
 *   DIV2 alone is insufficient.  Solution: use the TCA0 prescaler output
 *   as TCB0's clock source (CLK_TCA).  TCA0 is configured as a simple
 *   free-running prescaler (not used for PWM/waveform).
 *
 *   TCA0 prescaler DIV1024:
 *     TCA clock fed to TCB0 = 4 000 000 / 1024 = 3906.25 Hz  (? 3906 Hz)
 *
 *   For exactly 1 second we need a non-integer result, so we split the
 *   period across TCA0's prescaler and TCB0's CCMP:
 *
 *   Better approach — CLK_PER with TCA DIV256 + TCB DIV2:
 *     Effective TCB clock = 4 000 000 / 256 / 2  ... still not integer
 *
 *   Cleanest integer solution:
 *     TCA0 prescaler = DIV64
 *     TCB0 source    = CLK_TCA
 *     TCB clock      = 4 000 000 / 64 = 62 500 Hz
 *     CCMP           = 62 500 - 1 = 62 499  (fits in 16 bits, max 65535) ?
 *     Period         = 62 500 / 62 500 Hz = 1.000 000 s  ?
 *
 *   Register summary:
 *     TCA0.SINGLE.CTRLA  = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm
 *     TCB0.CCMP          = 62499
 *     TCB0.CTRLA         = TCB_CLKSEL_TCA0_gc | TCB_ENABLE_bm
 *     TCB0.CTRLB         = TCB_CNTMODE_INT_gc
 *     TCB0.INTCTRL       = TCB_CAPT_bm
 *
 * Sleep mode:
 *   Standby sleep is used instead of Power-Down because TCB0 is clocked
 *   from CLK_PER (synchronous domain).  In Power-Down the main clock is
 *   stopped and TCB0 would freeze.  In Standby sleep, peripherals that
 *   have RUNSTDBY set (or are in the synchronous domain with the
 *   STANDBY bit) continue operating.
 *   ? TCB0.CTRLA |= TCB_RUNSTDBY_bm keeps TCB0 ticking in Standby.
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
#define LED_PIN             PIN6_bm     /* PC6 – LED0 on Curiosity Nano */

/* CLK_PER = 4 MHz (default OSCHF after reset)
 * TCA0 prescaler = DIV64  ?  TCB clock = 4 000 000 / 64 = 62 500 Hz
 * CCMP = 62 500 - 1 = 62 499  ?  period = 1.000 000 s               */
#define TCB0_CCMP_1S        ((uint16_t)62499u)

/* ------------------------------------------------------------------ */
/* Prototypes                                                          */
/* ------------------------------------------------------------------ */
static void LED_init(void);
static void TCA0_init(void);
static void TCB0_init(void);
static void SLEEP_init(void);

/* ================================================================== */
/* Interrupt Service Routine                                           */
/* ================================================================== */
/**
 * TCB0 Capture / Periodic Interrupt – fires every 1 second.
 * Toggle the LED, clear the interrupt flag, return to sleep.
 */
ISR(TCB0_INT_vect)
{
    /* Clear the interrupt flag (write 1 to clear) */
    TCB0.INTFLAGS = TCB_CAPT_bm;

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
    PORTC.DIRSET = LED_PIN;     /* PC6 as output     */
    PORTC.OUTSET = LED_PIN;     /* LED off initially */
}

/**
 * @brief Configure TCA0 as a free-running prescaler for TCB0.
 *
 * TCA0 is not used for waveform generation here; it serves purely as a
 * clock divider.  The DIV64 prescaler reduces 4 MHz ? 62 500 Hz which
 * is fed to TCB0 via the CLK_TCA source selection.
 *
 * TCA0 is left in Normal (free-running) mode with ENABLE set.
 * RUNSTDBY is set so TCA0 continues counting during Standby sleep.
 */
static void TCA0_init(void)
{
    /* Prescaler DIV64, enable TCA0, run in Standby sleep */
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc
                      | TCA_SINGLE_ENABLE_bm
                      | TCA_SINGLE_RUNSTDBY_bm;
}

/**
 * @brief Configure TCB0 in Periodic Interrupt mode for a 1-second period.
 *
 * Steps:
 *  1. Set CCMP to 62499 (period = 62500 TCA-clocks = 1 s at 62.5 kHz).
 *  2. Select Periodic Interrupt mode (CNTMODE = INT).
 *  3. Enable the CAPT interrupt.
 *  4. Select CLK_TCA as clock source, enable TCB0, set RUNSTDBY.
 *
 * In Periodic Interrupt mode (INT):
 *   - CNT counts 0 ? CCMP
 *   - On CNT == CCMP: CAPT interrupt flag is set, CNT resets to 0
 *   - No manual CCMP update needed in the ISR
 */
static void TCB0_init(void)
{
    /* 1. Period: (62500 - 1) TCA-clock ticks = 1 second */
    TCB0.CCMP    = TCB0_CCMP_1S;

    /* 2. Periodic Interrupt mode */
    TCB0.CTRLB   = TCB_CNTMODE_INT_gc;

    /* 3. Enable CAPT interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* 4. Clock = CLK_TCA (TCA0 prescaler output), enable, run in Standby */
    TCB0.CTRLA   = TCB_CLKSEL_TCA0_gc
                 | TCB_ENABLE_bm
                 | TCB_RUNSTDBY_bm;
}

/**
 * @brief Select Standby as the sleep mode.
 *
 * TCB0 is driven by the synchronous CLK_PER domain via TCA0.
 * Power-Down stops CLK_PER, which would freeze TCB0.
 * Standby sleep keeps the synchronous clocks running for peripherals
 * that have RUNSTDBY set (TCA0 and TCB0 both do here).
 */
static void SLEEP_init(void)
{
    SLPCTRL.CTRLA = SLPCTRL_SMODE_STDBY_gc  /* Standby mode */
                  | SLPCTRL_SEN_bm;          /* Sleep enable */
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */
int main(void)
{
	// Configure PC6 as a push-pull output for LED0.
	LED_init();
	
	// Configure TCA0 as a free-running prescaler for TCB0.
	TCA0_init();
	
	// Configure TCB0 in Periodic Interrupt mode for a 1-second period.
	TCB0_init();
	
	// Select Standby as the sleep mode.
	SLEEP_init();

	// Enable Global interrupt
	sei();          

	for (;;)
	{
		// Enter Standby; TCB0 keeps running, wakes on CAPT INT
		sleep_cpu(); 
	}
}