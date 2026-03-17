/**
 * @file main.c
 * @brief TCA0 PWM on WO0 -> AC0 -> EVSYS -> TCB0 capture to measure frequency.
 *
 * Lab:
 * - Generate PWM with TCA0 WO0 at a known frequency (50% duty).
 * - Use AC0 as an event generator that detects PWM rising edges (WO0 wired into AC input).
 * - Route AC0 output events through EVSYS to TCB0 capture.
 * - Use TCB0 capture ISR to compute PWM frequency and store it in g_wo0_freq_hz.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "board.h"

/* Global measured frequency (Hz), updated in the TCB0 ISR. */
volatile uint32_t g_wo0_freq_hz = 0u;

/* Capture history for period measurement. */
static volatile uint16_t g_last_capt = 0u;
static volatile uint8_t  g_have_last = 0u;

/**
 * @brief Initialize DAC0 to mid-scale (about VDD/2) for the comparator threshold.
 */
static void dac0_init_midscale(void)
{
    /* Disable DAC before config */
    DAC0.CTRLA = 0;

    /* Mid-scale for 10-bit DAC (512) */
    DAC0.DATA = 0x0200;

    /* Enable DAC */
    DAC0.CTRLA = DAC_ENABLE_bm;
}

/**
 * @brief Initialize TCA0 in single-slope PWM mode on WO0 with 50% duty.
 *
 * @param pwm_hz Desired PWM frequency in Hz.
 */
static void tca0_pwm_init_wo0(uint32_t pwm_hz)
{
    uint32_t top;

    /* Route TCA0 outputs */
#ifdef PORTMUX_TCAROUTEA
    PORTMUX.TCAROUTEA = TCA0_ROUTE_GC;
#endif

    /* Disable before config */
    TCA0.SINGLE.CTRLA = 0;

    /* Single-slope PWM, enable CMP0 (WO0) */
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc
                      | TCA_SINGLE_CMP0EN_bm;

    /* Protect against divide by zero */
    if (pwm_hz == 0u)
    {
        pwm_hz = 1u;
    }

    /* TOP = (F_CPU / pwm_hz) - 1 */
    top = (uint32_t)(F_CPU / pwm_hz);
    if (top != 0u)
    {
        top -= 1u;
    }

    if (top > 0xFFFFu)
    {
        top = 0xFFFFu;
    }

    /* Set period and 50% duty */
    TCA0.SINGLE.PER = (uint16_t)top;
    TCA0.SINGLE.CMP0 = (uint16_t)((top + 1u) / 2u);

    /* Enable with DIV1 prescaler */
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc
                      | TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Initialize AC0 to compare external input (WO0 wired in) vs DAC0 threshold.
 */
static void ac0_init_event_generator(void)
{
    /* Disable AC0 before config */
    AC0.CTRLA = 0;

    /* Select POS and NEG inputs */
    AC0.MUXCTRL = (uint8_t)(AC0_POS_MUX_GC | AC_MUXNEG_DACREF_gc);

    /* Optional hysteresis to reduce chatter */
#ifdef AC_HYSMODE_SMALL_gc
    AC0.CTRLA |= AC_HYSMODE_SMALL_gc;
#endif

    /* Enable output if supported */
#ifdef AC_OUTEN_bm
    AC0.CTRLA |= AC_OUTEN_bm;
#endif

    /* Enable comparator */
    AC0.CTRLA |= AC_ENABLE_bm;
}

/**
 * @brief Initialize EVSYS to route AC0 output (rising edge) to TCB0 capture.
 */
static void evsys_init_ac0_to_tcb0(void)
{
#if defined(EVSYS_CHANNEL0)
	#if defined(EVSYS_CHANNEL0_AC0_OUT_gc) && defined(EVSYS_CHANNEL_EDGSEL_RISING_gc)
		EVSYS.CHANNEL0 = (uint8_t)(EVSYS_CHANNEL0_AC0_OUT_gc | EVSYS_CHANNEL_EDGSEL_RISING_gc);
	#elif defined(EVSYS_CHANNEL0_AC0_OUT_gc)
		EVSYS.CHANNEL0 = (uint8_t)(EVSYS_CHANNEL0_AC0_OUT_gc);
	#else
		EVSYS.CHANNEL0 = 0;
	#endif
#endif

/* Connect the event user (TCB0) to event channel 0 */
#if defined(EVSYS_USERTCB0CAPT)
    EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL0_gc;
#elif defined(EVSYS_USERTCB0)
    EVSYS.USERTCB0 = EVSYS_USER_CHANNEL0_gc;
#endif
}

/**
 * @brief Initialize TCB0 to capture timer count on EVSYS events and generate interrupts.
 */
static void tcb0_capture_init(void)
{
    /* Disable before config */
    TCB0.CTRLA = 0;

    /* Capture mode */
#ifdef TCB_CNTMODE_CAPT_gc
    TCB0.CTRLB = TCB_CNTMODE_CAPT_gc;
#else
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
#endif

    /* Enable event input for capture */
#ifdef TCB_CAPTEI_bm
    TCB0.EVCTRL = TCB_CAPTEI_bm;
#else
    TCB0.EVCTRL = 1;
#endif

    /* Clear flag and enable interrupt */
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Enable timer, CLKDIV1 if available */
#ifdef TCB_CLKSEL_DIV1_gc
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;
#else
    TCB0.CTRLA = TCB_ENABLE_bm;
#endif
}

/**
 * @brief TCB0 capture ISR computes PWM frequency from captured tick deltas.
 */
ISR(TCB0_INT_vect)
{
    uint16_t capt;
    uint16_t dt;

    /* Clear interrupt flag */
    TCB0.INTFLAGS = TCB_CAPT_bm;

    /* Read captured value */
    capt = TCB0.CCMP;

    /* First capture just seeds history */
    if (g_have_last == 0u)
    {
        g_last_capt = capt;
        g_have_last = 1u;
        g_wo0_freq_hz = 0u;
        return;
    }

    /* Delta in ticks (wrap-safe) */
    dt = (uint16_t)(capt - g_last_capt);
    g_last_capt = capt;

    if (dt == 0u)
    {
        g_wo0_freq_hz = 0u;
        return;
    }

    /* Frequency = F_CPU / dt */
    g_wo0_freq_hz = (uint32_t)(F_CPU / (uint32_t)dt);
}

/**
 * @brief Main entry point.
 */
int main(void)
{
    /* Optional LED */
    board_led_init();

    /* Threshold for comparator */
    dac0_init_midscale();

    /* PWM generation */
    tca0_pwm_init_wo0(PWM_TARGET_HZ);

    /* Comparator as event generator */
    ac0_init_event_generator();

    /* EVSYS routing */
    evsys_init_ac0_to_tcb0();

    /* TCB capture user */
    tcb0_capture_init();

    /* Enable interrupts */
    sei();

    while (1)
    {
        /* Heartbeat delay loop (keeps CPU busy, frequency is measured in ISR) */
        board_led_toggle();
        for (volatile uint32_t i = 0; i < 200000UL; i++)
        {
            __asm__ __volatile__("nop");
        }
    }
}