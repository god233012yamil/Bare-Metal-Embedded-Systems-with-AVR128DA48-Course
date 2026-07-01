#include <stdint.h>
#include <string.h>
#include "app_events.h"
#include "board.h"
#include "button.h"
#include "command_dispatcher.h"
#include "led_driver.h"
#include "soft_timer.h"
#include "system_state_machine.h"
#include "uart_driver.h"

#define APP_COMMAND_BUFFER_SIZE 32U

static system_context_t app_context;
static button_t user_button;
static soft_timer_t heartbeat_timer;
static command_dispatcher_t command_dispatcher;
static char command_buffer[APP_COMMAND_BUFFER_SIZE];
static uint8_t command_index = 0U;
static volatile uint8_t system_tick_pending = 0U;

static void command_enable(const char *args, void *context);
static void command_disable(const char *args, void *context);
static void command_fault(const char *args, void *context);
static void command_clear_fault(const char *args, void *context);
static void command_status(const char *args, void *context);
static void heartbeat_callback(void *context);

static const led_interface_t status_led =
{
    .write = board_led0_write,
    .toggle = board_led0_toggle
};

static const command_entry_t command_table[] =
{
    { "ENABLE", command_enable },
    { "DISABLE", command_disable },
    { "FAULT", command_fault },
    { "CLEAR", command_clear_fault },
    { "STATUS", command_status }
};

/**
 * Enables the application state machine from a command handler.
 *
 * Args:
 *     args: Optional command arguments. Unused by this command.
 *     context: Pointer to system_context_t.
 */
static void command_enable(const char *args, void *context)
{
    system_context_t *system = (system_context_t *)context;
    (void)args;

    if (system != 0)
    {
        system->enabled = 1U;
    }

    uart0_write_string("OK ENABLE\r\n");
}

/**
 * Disables the application state machine from a command handler.
 *
 * Args:
 *     args: Optional command arguments. Unused by this command.
 *     context: Pointer to system_context_t.
 */
static void command_disable(const char *args, void *context)
{
    system_context_t *system = (system_context_t *)context;
    (void)args;

    if (system != 0)
    {
        system->enabled = 0U;
    }

    uart0_write_string("OK DISABLE\r\n");
}

/**
 * Requests the fault state from a command handler.
 *
 * Args:
 *     args: Optional command arguments. Unused by this command.
 *     context: Pointer to system_context_t.
 */
static void command_fault(const char *args, void *context)
{
    system_context_t *system = (system_context_t *)context;
    (void)args;

    if (system != 0)
    {
        system->fault_requested = 1U;
    }

    uart0_write_string("OK FAULT\r\n");
}

/**
 * Clears the fault request from a command handler.
 *
 * Args:
 *     args: Optional command arguments. Unused by this command.
 *     context: Pointer to system_context_t.
 */
static void command_clear_fault(const char *args, void *context)
{
    system_context_t *system = (system_context_t *)context;
    (void)args;

    if (system != 0)
    {
        system->fault_requested = 0U;
    }

    uart0_write_string("OK CLEAR\r\n");
}

/**
 * Sends a compact status message from a command handler.
 *
 * Args:
 *     args: Optional command arguments. Unused by this command.
 *     context: Pointer to system_context_t.
 */
static void command_status(const char *args, void *context)
{
    system_context_t *system = (system_context_t *)context;
    (void)args;

    uart0_write_string("STATE=");

    if (system_state_machine_get_state() == SYSTEM_STATE_IDLE)
    {
        uart0_write_string("IDLE");
    }
    else if (system_state_machine_get_state() == SYSTEM_STATE_ACTIVE)
    {
        uart0_write_string("ACTIVE");
    }
    else
    {
        uart0_write_string("FAULT");
    }

    if (system != 0)
    {
        uart0_write_string(system->enabled != 0U ? " ENABLED" : " DISABLED");
    }

    uart0_write_string("\r\n");
}

/**
 * Toggles the heartbeat LED from the software timer callback.
 *
 * Args:
 *     context: Optional user context. Unused by this callback.
 */
static void heartbeat_callback(void *context)
{
    (void)context;
    board_led1_toggle();
}

/**
 * Initializes the application modules and demonstrates function pointer wiring.
 */
void app_init(void)
{
    system_state_machine_init(&app_context);

    led_driver_init(&status_led);

    button_init(&user_button,
                board_read_button,
                app_button_event_handler,
                &app_context,
                5U);

    soft_timer_init(&heartbeat_timer,
                    100U,
                    heartbeat_callback,
                    0);
    soft_timer_start(&heartbeat_timer);

    command_dispatcher_init(&command_dispatcher,
                            command_table,
                            (uint8_t)(sizeof(command_table) / sizeof(command_table[0])),
                            &app_context);

    uart0_init(BOARD_UART_BAUD_RATE);
    uart0_register_rx_callback(app_uart_rx_handler, &command_dispatcher);
    uart0_write_string("Function pointer demo ready\r\n");
}

/**
 * Processes periodic application work from the main loop.
 */
void app_process(void)
{
    if (system_tick_pending != 0U)
    {
        system_tick_pending = 0U;
        button_process(&user_button);
        soft_timer_tick(&heartbeat_timer);
    }

    system_state_machine_run(&app_context);
}

/**
 * Handles debounced button events through a callback.
 *
 * Args:
 *     event: Button event type.
 *     context: Pointer to system_context_t.
 */
void app_button_event_handler(button_event_t event, void *context)
{
    system_context_t *system = (system_context_t *)context;

    if ((event == BUTTON_EVENT_PRESSED) && (system != 0))
    {
        system->enabled ^= 1U;
        system->button_press_count++;
    }
}

/**
 * Receives UART bytes from the USART driver callback and dispatches commands.
 *
 * Args:
 *     data: Received UART byte.
 *     context: Pointer to command_dispatcher_t.
 */
void app_uart_rx_handler(uint8_t data, void *context)
{
    command_dispatcher_t *dispatcher = (command_dispatcher_t *)context;

    if ((data == '\r') || (data == '\n'))
    {
        if (command_index > 0U)
        {
            command_buffer[command_index] = '\0';

            if (command_dispatcher_process(dispatcher, command_buffer) == 0U)
            {
                uart0_write_string("ERR UNKNOWN\r\n");
            }

            command_index = 0U;
            memset(command_buffer, 0, sizeof(command_buffer));
        }
    }
    else if (command_index < (APP_COMMAND_BUFFER_SIZE - 1U))
    {
        command_buffer[command_index] = (char)data;
        command_index++;
    }
    else
    {
        command_index = 0U;
        memset(command_buffer, 0, sizeof(command_buffer));
        uart0_write_string("ERR LONG\r\n");
    }
}

/**
 * Flags that a one-millisecond system tick has occurred.
 */
void app_request_tick(void)
{
    system_tick_pending = 1U;
}
