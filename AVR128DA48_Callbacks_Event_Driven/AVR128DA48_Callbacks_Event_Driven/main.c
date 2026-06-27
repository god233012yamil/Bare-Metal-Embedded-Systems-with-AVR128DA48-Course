#include "app_event.h"
#include "board.h"
#include "button.h"
#include "software_timer.h"
#include "system_tick.h"

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LED_ON_TIME_MS 1000UL

/**
 * @brief Posts a button event from the button driver's interrupt callback.
 *
 * Args:
 *     button_id: Identifier supplied by the button driver.
 *     context: Optional application context. Unused in this example.
 */
static void app_button_callback(uint8_t button_id, void *context)
{
    const app_event_t event =
    {
        .type = APP_EVENT_BUTTON_PRESSED,
        .data = button_id
    };

    (void)context;
    (void)app_event_push_from_isr(event);
}

/**
 * @brief Posts a timeout event from the software timer callback.
 *
 * Args:
 *     context: Optional application context. Unused in this example.
 */
static void app_timeout_callback(void *context)
{
    const app_event_t event =
    {
        .type = APP_EVENT_TIMEOUT,
        .data = 0U
    };

    (void)context;
    (void)app_event_push_from_isr(event);
}

/**
 * @brief Processes one event in main-loop context.
 *
 * Args:
 *     event: Event to process.
 */
static void app_process_event(const app_event_t *event)
{
    if (event == NULL)
    {
        return;
    }

    switch (event->type)
    {
        case APP_EVENT_BUTTON_PRESSED:
            board_led_toggle();
            (void)software_timer_start(LED_ON_TIME_MS,
                                       app_timeout_callback,
                                       NULL);
            break;

        case APP_EVENT_TIMEOUT:
            board_led_off();
            break;

        case APP_EVENT_NONE:
        default:
            break;
    }
}

/**
 * @brief Initializes all modules used by the callback demonstration.
 */
static void app_init(void)
{
    cli();

    board_init();
	
    app_event_init();
	
    button_init();
	
    software_timer_init();
	
    system_tick_init();

    button_register_callback(app_button_callback, NULL);

    set_sleep_mode(SLEEP_MODE_IDLE);
	
    sei();
}

/**
 * @brief Application entry point.
 *
 * Returns:
 *     This function does not return.
 */
int main(void)
{
    app_event_t event;

    // Initializes all modules used by the callback demonstration.
	app_init();

    for (;;)
    {
        // Removes the oldest event from the queue.
		if (app_event_pop(&event))
        {
            // Processes one event in main-loop context.
			app_process_event(&event);
        }
        else
        {
            // Idle sleep leaves TCB0 and PORT interrupts operational.
            sleep_mode();
        }
    }
}