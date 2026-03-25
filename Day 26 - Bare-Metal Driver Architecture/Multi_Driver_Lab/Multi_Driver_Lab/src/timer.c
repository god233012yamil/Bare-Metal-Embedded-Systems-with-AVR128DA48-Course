/*
 * timer.c  --  TCB driver implementation, AVR128DA48
 *
 * Peripherals : TCB0 (0x0B00), TCB1 (0x0B10), TCB2 (0x0B20), TCB3 (0x0B30)
 * All register symbols from ioavr128da48.h via <avr/io.h>.
 *
 * TCB.CTRLA
 *   bit 0     : TCB_ENABLE_bm
 *   bits 3:1  : TCB_CLKSEL_gm  (TCB_CLKSEL_DIV1_gc | TCB_CLKSEL_DIV2_gc)
 *
 * TCB.CTRLB
 *   bits 2:0  : TCB_CNTMODE_gm  (TCB_CNTMODE_INT_gc = 0x00 = periodic interrupt)
 *
 * TCB.INTCTRL / TCB.INTFLAGS
 *   bit 0     : TCB_CAPT_bm  (capture / period-match)
 *   bit 1     : TCB_OVF_bm   (overflow -- not used in INT mode)
 *
 * Interrupt vectors (from ioavr128da48.h):
 *   TCB0_INT_vect   _VECTOR(12)
 *   TCB1_INT_vect   _VECTOR(13)
 *   TCB2_INT_vect   _VECTOR(30)
 *   TCB3_INT_vect   _VECTOR(41)
 */

#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "stddef.h"  // NULL is defined here

/* -----------------------------------------------------------------------
 * Internal state
 * ----------------------------------------------------------------------- */
static timer_isr_callback_t prv_callbacks[4] = { NULL, NULL, NULL, NULL };

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */
static TCB_t *prv_inst(timer_instance_t inst)
{
    switch (inst) {
        case TIMER_TCB0: return &TCB0;
        case TIMER_TCB1: return &TCB1;
        case TIMER_TCB2: return &TCB2;
        case TIMER_TCB3: return &TCB3;
        default:         return &TCB0;
    }
}

static uint8_t prv_clksel_gc(timer_clksel_t clksel)
{
    return (clksel == TIMER_CLK_DIV2) ? TCB_CLKSEL_DIV2_gc : TCB_CLKSEL_DIV1_gc;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

uint16_t TIMER_UsToCompare(uint32_t period_us, uint32_t f_cpu_hz,
                           timer_clksel_t clksel)
{
    uint32_t divisor = (clksel == TIMER_CLK_DIV2) ? 2UL : 1UL;
    uint32_t ticks   = (period_us * (f_cpu_hz / divisor)) / 1000000UL;
    if (ticks == 0) ticks = 1;
    ticks -= 1;
    if (ticks > 0xFFFFUL) ticks = 0xFFFFUL;
    return (uint16_t)ticks;
}

void TIMER_RegisterCallback(timer_instance_t inst, timer_isr_callback_t cb)
{
    if (inst <= TIMER_TCB3) {
        prv_callbacks[(uint8_t)inst] = cb;
    }
}

void TIMER_Init(const timer_config_t *cfg)
{
    TCB_t *tcb = prv_inst(cfg->instance);

    /* Disable before reconfiguring */
    tcb->CTRLA = 0;

    /* Periodic Interrupt mode */
    tcb->CTRLB = TCB_CNTMODE_INT_gc;

    /* Load compare/period value */
    tcb->CCMP = cfg->period_ticks;

    /* Clear any pending flag */
    tcb->INTFLAGS = TCB_CAPT_bm;

    /* Configure interrupt */
    tcb->INTCTRL = cfg->interrupt_enable ? TCB_CAPT_bm : 0;

    /* Set clock source and enable */
    tcb->CTRLA = prv_clksel_gc(cfg->clksel) | TCB_ENABLE_bm;
}

void TIMER_Start(timer_instance_t inst)
{
    prv_inst(inst)->CTRLA |= TCB_ENABLE_bm;
}

void TIMER_Stop(timer_instance_t inst)
{
    prv_inst(inst)->CTRLA &= ~TCB_ENABLE_bm;
}

void TIMER_SetPeriod(timer_instance_t inst, uint16_t ticks)
{
    prv_inst(inst)->CCMP = ticks;
}

uint16_t TIMER_GetCount(timer_instance_t inst)
{
    return prv_inst(inst)->CNT;
}

uint8_t TIMER_PollFlag(timer_instance_t inst)
{
    TCB_t *tcb = prv_inst(inst);
    if (tcb->INTFLAGS & TCB_CAPT_bm) {
        tcb->INTFLAGS = TCB_CAPT_bm;  /* Clear by writing 1 */
        return 1;
    }
    return 0;
}

void TIMER_ClearFlag(timer_instance_t inst)
{
    prv_inst(inst)->INTFLAGS = TCB_CAPT_bm;
}

/* -----------------------------------------------------------------------
 * ISR handlers — call the registered callback if present
 * ----------------------------------------------------------------------- */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    if (prv_callbacks[TIMER_TCB0]) {
        prv_callbacks[TIMER_TCB0](TIMER_TCB0);
    }
}

ISR(TCB1_INT_vect)
{
    TCB1.INTFLAGS = TCB_CAPT_bm;
    if (prv_callbacks[TIMER_TCB1]) {
        prv_callbacks[TIMER_TCB1](TIMER_TCB1);
    }
}

ISR(TCB2_INT_vect)
{
    TCB2.INTFLAGS = TCB_CAPT_bm;
    if (prv_callbacks[TIMER_TCB2]) {
        prv_callbacks[TIMER_TCB2](TIMER_TCB2);
    }
}

ISR(TCB3_INT_vect)
{
    TCB3.INTFLAGS = TCB_CAPT_bm;
    if (prv_callbacks[TIMER_TCB3]) {
        prv_callbacks[TIMER_TCB3](TIMER_TCB3);
    }
}
