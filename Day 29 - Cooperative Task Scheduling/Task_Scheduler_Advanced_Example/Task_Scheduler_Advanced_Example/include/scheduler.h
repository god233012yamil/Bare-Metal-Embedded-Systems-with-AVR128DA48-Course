#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/* Scheduler tick period = 1 ms (set by TCB0) */
#define TICK_PERIOD_MS          1u

/* Task periods in ticks (1 tick = 1 ms) */
#define TASK_ADC_PERIOD_MS      20u
#define TASK_COUNTER_PERIOD_MS  1000u
#define TASK_BUTTON_PERIOD_MS   10u
#define TASK_LED_PERIOD_MS      200u

/* Task flag bits */
#define FLAG_ADC_SAMPLE         (1u << 0)
#define FLAG_COUNTER_UPDATE     (1u << 1)
#define FLAG_BUTTON_CHECK       (1u << 2)
#define FLAG_LED_TOGGLE         (1u << 3)

/* Global tick counter - updated in TCB0 ISR */
extern volatile uint32_t g_tick_ms;

/* Task flags - set in TCB0 ISR, cleared by scheduler after task runs */
extern volatile uint8_t g_task_flags;

void scheduler_init(void);
void scheduler_run(void);

#endif /* SCHEDULER_H */
