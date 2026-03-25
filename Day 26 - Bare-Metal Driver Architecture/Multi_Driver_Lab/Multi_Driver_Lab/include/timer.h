/*
 * timer.h  --  TCB (Timer/Counter Type B) driver public API, AVR128DA48
 *
 * The AVR128DA48 has four TCB instances: TCB0 – TCB3.
 * This driver targets the most common use case: Periodic Interrupt mode
 * (TCB_CNTMODE_INT_gc), where the timer counts from 0 up to CCMP and
 * generates an interrupt / sets the CAPT flag on each match.
 *
 * The timer period in microseconds is:
 *
 *   period_us = (CCMP + 1) * clock_divider / F_CPU_Hz * 1 000 000
 *
 * A convenience helper TIMER_UsToCompare() computes the CCMP value for a
 * requested period in microseconds.
 *
 * Interrupt-driven usage:
 *   1. Implement the ISR (see timer_isr_callback_t below or use the raw
 *      TCBn_INT_vect vectors).
 *   2. Call TIMER_RegisterCallback() before TIMER_Init().
 *   3. Call TIMER_Init() with interrupt_enable = 1.
 *   4. Call sei() to enable global interrupts.
 *
 * Polled usage:
 *   1. Call TIMER_Init() with interrupt_enable = 0.
 *   2. Call TIMER_PollFlag() in the application loop; it returns 1 when the
 *      period has elapsed and clears the flag automatically.
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

/* TCB instance selector */
typedef enum {
    TIMER_TCB0 = 0,  /* TCB0 at 0x0B00, vector TCB0_INT_vect */
    TIMER_TCB1,      /* TCB1 at 0x0B10, vector TCB1_INT_vect */
    TIMER_TCB2,      /* TCB2 at 0x0B20, vector TCB2_INT_vect */
    TIMER_TCB3,      /* TCB3 at 0x0B30, vector TCB3_INT_vect */
} timer_instance_t;

/* Clock source for TCB (TCB_CLKSEL_*_gc written to TCB.CTRLA bits [3:1]) */
typedef enum {
    TIMER_CLK_DIV1 = 0x00,  /* TCB_CLKSEL_DIV1_gc  -- CLK_PER     */
    TIMER_CLK_DIV2 = 0x01,  /* TCB_CLKSEL_DIV2_gc  -- CLK_PER / 2 */
} timer_clksel_t;

/* Callback type used when interrupt mode is enabled */
typedef void (*timer_isr_callback_t)(timer_instance_t inst);

/* Driver configuration */
typedef struct {
    timer_instance_t    instance;
    timer_clksel_t      clksel;
    uint16_t            period_ticks;    /* Value loaded into TCB.CCMP  */
    uint8_t             interrupt_enable; /* Non-zero: enable CAPT interrupt */
} timer_config_t;

/*
 * Public API
 */

/**
 * @brief  Compute the TCB CCMP value for a desired period in microseconds.
 *
 *   ticks = (period_us * (F_CPU_Hz / divisor) / 1 000 000) - 1
 *
 * @param  period_us   Desired period in microseconds.
 * @param  f_cpu_hz    CPU clock frequency in Hz (e.g. 4000000UL).
 * @param  clksel      Clock divider selection.
 * @return 16-bit CCMP value (saturates to 0xFFFF if period is too long).
 */
uint16_t TIMER_UsToCompare(uint32_t period_us, uint32_t f_cpu_hz,
                           timer_clksel_t clksel);

/**
 * @brief  Register an ISR callback for a TCB instance.
 *         Call before TIMER_Init(); the callback is invoked from the TCB ISR.
 * @param  inst  Timer instance.
 * @param  cb    Callback function pointer (NULL to unregister).
 */
void TIMER_RegisterCallback(timer_instance_t inst, timer_isr_callback_t cb);

/**
 * @brief  Initialise a TCB in Periodic Interrupt mode.
 * @param  cfg  Pointer to a filled timer_config_t.
 */
void TIMER_Init(const timer_config_t *cfg);

/**
 * @brief  Start the timer (sets TCB_ENABLE_bm in TCB.CTRLA).
 * @param  inst  Timer instance.
 */
void TIMER_Start(timer_instance_t inst);

/**
 * @brief  Stop the timer (clears TCB_ENABLE_bm).
 * @param  inst  Timer instance.
 */
void TIMER_Stop(timer_instance_t inst);

/**
 * @brief  Change the compare/period value at runtime without re-initialising.
 * @param  inst    Timer instance.
 * @param  ticks   New CCMP value.
 */
void TIMER_SetPeriod(timer_instance_t inst, uint16_t ticks);

/**
 * @brief  Read the current 16-bit counter value (TCB.CNT).
 * @param  inst  Timer instance.
 * @return Current counter value.
 */
uint16_t TIMER_GetCount(timer_instance_t inst);

/**
 * @brief  Poll the CAPT interrupt flag (TCB_CAPT_bm in TCB.INTFLAGS).
 *         Clears the flag if it was set.
 * @param  inst  Timer instance.
 * @return 1 if the period has elapsed since last call, 0 otherwise.
 */
uint8_t TIMER_PollFlag(timer_instance_t inst);

/**
 * @brief  Clear the CAPT interrupt flag without reading it.
 * @param  inst  Timer instance.
 */
void TIMER_ClearFlag(timer_instance_t inst);

#endif /* TIMER_H_ */
