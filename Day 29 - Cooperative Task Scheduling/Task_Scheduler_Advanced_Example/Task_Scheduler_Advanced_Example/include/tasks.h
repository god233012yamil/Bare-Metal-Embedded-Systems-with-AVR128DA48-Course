#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  LED State Machine States                                            */
/* ------------------------------------------------------------------ */
typedef enum
{
    LED_STATE_IDLE       = 0,   /* LED off - waiting for first button press  */
    LED_STATE_BLINK      = 1,   /* Normal blink - 200 ms period              */
    LED_STATE_FAST_BLINK = 2    /* Fast blink   - every 200 ms tick = ~5 Hz  */
                                /* (the LED task fires on every scheduler    */
                                /*  tick, so fast = toggle every call)       */
} led_state_t;

/* Exposed so UART reporting can read them */
extern uint16_t          g_adc_result;
extern uint32_t          g_second_counter;
extern bool              g_button_pressed;
extern led_state_t       g_led_state;

/* Task entry points called by scheduler_run() */
void task_adc_sample(void);
void task_counter_update(void);
void task_button_check(void);
void task_led_toggle(void);

#endif /* TASKS_H */
