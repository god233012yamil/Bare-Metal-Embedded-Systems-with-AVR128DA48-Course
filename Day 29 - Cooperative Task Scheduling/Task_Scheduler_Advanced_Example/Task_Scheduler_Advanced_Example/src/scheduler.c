#include "scheduler.h"
#include "tasks.h"
#include <avr/interrupt.h>

volatile uint32_t g_tick_ms   = 0;
volatile uint8_t  g_task_flags = 0;

static uint32_t s_last_adc     = 0;
static uint32_t s_last_counter = 0;
static uint32_t s_last_button  = 0;
static uint32_t s_last_led     = 0;

/*
 * TCB0 ISR - fires every 1 ms
 * Increments global tick and sets task flags at the required intervals.
 */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;

    g_tick_ms++;

    if ((g_tick_ms - s_last_adc) >= TASK_ADC_PERIOD_MS)
    {
        s_last_adc = g_tick_ms;
        g_task_flags |= FLAG_ADC_SAMPLE;
    }

    if ((g_tick_ms - s_last_counter) >= TASK_COUNTER_PERIOD_MS)
    {
        s_last_counter = g_tick_ms;
        g_task_flags |= FLAG_COUNTER_UPDATE;
    }

    if ((g_tick_ms - s_last_button) >= TASK_BUTTON_PERIOD_MS)
    {
        s_last_button = g_tick_ms;
        g_task_flags |= FLAG_BUTTON_CHECK;
    }

    if ((g_tick_ms - s_last_led) >= TASK_LED_PERIOD_MS)
    {
        s_last_led = g_tick_ms;
        g_task_flags |= FLAG_LED_TOGGLE;
    }
}

void scheduler_init(void)
{
    g_tick_ms    = 0;
    g_task_flags = 0;
    s_last_adc     = 0;
    s_last_counter = 0;
    s_last_button  = 0;
    s_last_led     = 0;
}

/*
 * scheduler_run() - called repeatedly from main().
 * Checks flags atomically and dispatches the corresponding task handler.
 * Each flag is cleared before calling the task so new flags can be set
 * by the ISR while the task is executing.
 */
void scheduler_run(void)
{
    uint8_t flags;

    /* Atomically snapshot and clear pending flags */
    cli();
    flags        = g_task_flags;
    g_task_flags = 0;
    sei();

    if (flags & FLAG_ADC_SAMPLE)
    {
        task_adc_sample();
    }

    if (flags & FLAG_COUNTER_UPDATE)
    {
        task_counter_update();
    }

    if (flags & FLAG_BUTTON_CHECK)
    {
        task_button_check();
    }

    if (flags & FLAG_LED_TOGGLE)
    {
        task_led_toggle();
    }
}
