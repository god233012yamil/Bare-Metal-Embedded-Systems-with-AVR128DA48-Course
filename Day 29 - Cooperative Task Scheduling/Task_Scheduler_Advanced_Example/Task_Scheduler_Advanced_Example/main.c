/*
 * AVR128DA48 Cooperative Task Scheduler
 * Target: AVR128DA48 Curiosity Nano
 * IDE: Atmel/Microchip Studio 7
 * Device Pack: AVR-Dx 2.4.286
 *
 * Hardware Connections (Curiosity Nano defaults):
 *   LED0     -> PB3 (active LOW)
 *   SW0      -> PB2 (active LOW, internal pull-up)
 *   POT      -> PD0 / AIN0 (connect a 10k pot between VDD and GND, wiper to PD0)
 *
 * Scheduler tick: TCB0 @ 1 ms (3 MHz / 3000 = 1 kHz)
 * Tasks:
 *   task_adc_sample  - every 20 ms  - samples potentiometer via ADC0
 *   task_counter     - every 1000 ms - increments a 1-second software counter
 *   task_button      - every 10 ms   - debounces SW0 and detects press/release
 *   task_led         - every 200 ms  - toggles LED0 (state machine: IDLE -> BLINK -> FAST_BLINK)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#include "scheduler.h"
#include "tasks.h"
#include "tcb_timer.h"
#include "adc_driver.h"
#include "gpio.h"
#include "uart.h"

int main(void)
{
    // Initialize GPIOs for LED and Button
    gpio_init();
    
    // Initialize UART for debugging output
    uart_init();
    
    // Initialize ADC0 for potentiometer sampling
    adc_init();
    
    // Initialize TCB0 for 1 ms ticks
    tcb0_init();
    
    // Initialize the cooperative scheduler
    scheduler_init();

    // Enable global interrupts
    sei();

    uart_print_string("\r\n== AVR128DA48 Cooperative Scheduler ==\r\n");
    uart_print_string("Tick: 1 ms | Tasks: ADC(20ms) Counter(1s) Button(10ms) LED(200ms)\r\n\r\n");

    while (1)
    {
	    // Run the cooperative scheduler to execute ready tasks
	    scheduler_run();
    }
}