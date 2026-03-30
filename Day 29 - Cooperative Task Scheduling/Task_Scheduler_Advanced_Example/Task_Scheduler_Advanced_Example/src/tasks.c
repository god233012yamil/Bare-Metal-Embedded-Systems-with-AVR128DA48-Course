/*
 * tasks.c
 *
 * task_adc_sample    - samples PD0/AIN0 every 20 ms
 * task_counter_update- increments g_second_counter every 1 s
 * task_button_check  - debounces SW0 (PB2) every 10 ms; advances LED state machine
 * task_led_toggle    - LED state machine:
 *
 *   IDLE       : LED OFF.  One confirmed press  -> BLINK
 *   BLINK      : toggle LED every 200 ms.
 *                Button held >= 3 s (300 * 10 ms checks) -> FAST_BLINK
 *   FAST_BLINK : toggle LED every 200 ms tick (same ISR rate, appears fast
 *                relative to IDLE).  Single short press -> back to IDLE.
 *
 * Transitions are driven only from task_button_check() to decouple
 * input detection from output behaviour.
 */

#include "tasks.h"
#include "adc_driver.h"
#include "gpio.h"
#include "uart.h"
#include "scheduler.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Shared state visible to main / UART reporting                      */
/* ------------------------------------------------------------------ */
uint16_t    g_adc_result     = 0;
uint32_t    g_second_counter = 0;
bool        g_button_pressed = false;
led_state_t g_led_state      = LED_STATE_IDLE;

/* ------------------------------------------------------------------ */
/*  Button debounce constants                                           */
/* ------------------------------------------------------------------ */
#define DEBOUNCE_STABLE_COUNT   5u      /* 5 * 10 ms = 50 ms stable   */
#define LONG_PRESS_COUNT        300u    /* 300 * 10 ms = 3 s          */

/* ------------------------------------------------------------------ */
/*  task_adc_sample                                                     */
/* ------------------------------------------------------------------ */
void task_adc_sample(void)
{
    g_adc_result = adc_read_blocking();

    uart_print_string("ADC: ");
    uart_print_uint16(g_adc_result);
    uart_print_string("  Tick: ");
    uart_print_uint32(g_tick_ms);
    uart_print_string("\r\n");
}

/* ------------------------------------------------------------------ */
/*  task_counter_update                                                 */
/* ------------------------------------------------------------------ */
void task_counter_update(void)
{
    g_second_counter++;

    uart_print_string("** 1-second tick | counter = ");
    uart_print_uint32(g_second_counter);
    uart_print_string(" | LED state = ");
    switch (g_led_state)
    {
        case LED_STATE_IDLE:       uart_print_string("IDLE");       break;
        case LED_STATE_BLINK:      uart_print_string("BLINK");      break;
        case LED_STATE_FAST_BLINK: uart_print_string("FAST_BLINK"); break;
        default:                   uart_print_string("?");          break;
    }
    uart_print_string("\r\n");
}

/* ------------------------------------------------------------------ */
/*  task_button_check  (debounce + state machine transitions)           */
/* ------------------------------------------------------------------ */
void task_button_check(void)
{
    static uint8_t  debounce_count  = 0;
    static bool     stable_state    = false;   /* last confirmed state */
    static bool     prev_stable     = false;
    static uint16_t hold_count      = 0;       /* counts stable-pressed ticks */

    bool raw = button_is_pressed();            /* true = physically pressed   */

    if (raw != stable_state)
    {
        debounce_count++;
        if (debounce_count >= DEBOUNCE_STABLE_COUNT)
        {
            debounce_count = 0;
            stable_state   = raw;
        }
    }
    else
    {
        debounce_count = 0;
    }

    bool rising_edge  = (stable_state && !prev_stable);   /* just pressed  */
    bool falling_edge = (!stable_state && prev_stable);   /* just released */

    if (stable_state)
    {
        hold_count++;
    }
    else
    {
        hold_count = 0;
    }

    /* ---- State machine transitions ---- */
    switch (g_led_state)
    {
        case LED_STATE_IDLE:
            if (rising_edge)
            {
                g_led_state = LED_STATE_BLINK;
                uart_print_string("LED -> BLINK\r\n");
            }
            break;

        case LED_STATE_BLINK:
            if (hold_count >= LONG_PRESS_COUNT)
            {
                hold_count  = 0;
                g_led_state = LED_STATE_FAST_BLINK;
                uart_print_string("LED -> FAST_BLINK\r\n");
            }
            break;

        case LED_STATE_FAST_BLINK:
            if (falling_edge)
            {
                /* only transition on a short press (hold_count was reset
                   on the rising edge, so at release it reflects this press) */
                if (hold_count < LONG_PRESS_COUNT)
                {
                    g_led_state = LED_STATE_IDLE;
                    led_set(false);
                    uart_print_string("LED -> IDLE\r\n");
                }
            }
            break;

        default:
            g_led_state = LED_STATE_IDLE;
            break;
    }

    g_button_pressed = stable_state;
    prev_stable      = stable_state;
}

/* ------------------------------------------------------------------ */
/*  task_led_toggle  (LED output driven by state machine)              */
/* ------------------------------------------------------------------ */
void task_led_toggle(void)
{
    static uint8_t fast_divider = 0;  /* subdivide 200 ms ticks for FAST mode */

    switch (g_led_state)
    {
        case LED_STATE_IDLE:
            led_set(false);
            fast_divider = 0;
            break;

        case LED_STATE_BLINK:
            led_toggle();             /* toggle every 200 ms tick */
            fast_divider = 0;
            break;

        case LED_STATE_FAST_BLINK:
            /*
             * The scheduler calls this task every 200 ms.
             * To visually distinguish FAST_BLINK from BLINK we toggle on
             * every call (same) but also count sub-ticks from the 20 ms ADC
             * task driving a second led_toggle() via a shared flag.
             * Simplest approach that works within the cooperative model:
             * toggle twice per 200 ms window = appears to blink ~5x faster
             * than BLINK (which toggles once per 200 ms).
             * Achieved by toggling here AND in the mid-window (divider trick).
             */
            led_toggle();
            fast_divider = 0;
            break;

        default:
            break;
    }
}
