#include "hardware.h"

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 24000000UL
#endif

#define STATUS_LED_PIN PIN5_bm
#define BUTTON_PIN PIN6_bm
#define MOTOR_ENABLE_PIN PIN0_bm
#define FAULT_INPUT_PIN PIN1_bm

/**
 * Configures the internal high-frequency oscillator for 24 MHz operation.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void hardware_clock_init(void)
{
    // Protected writes are required for clock-controller registers.
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0U);
}

/**
 * Configures application GPIO directions, pull-ups, and safe output levels.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
static void hardware_gpio_init(void)
{
    // PF5 drives the Curiosity Nano LED and is active low.
    PORTF.OUTSET = STATUS_LED_PIN;
    PORTF.DIRSET = STATUS_LED_PIN;

    // PF6 is the Curiosity Nano push button and is active low.
    PORTF.DIRCLR = BUTTON_PIN;
    PORTF.PIN6CTRL = PORT_PULLUPEN_bm;

    // PC0 is a demonstration motor-enable output and starts disabled.
    PORTC.OUTCLR = MOTOR_ENABLE_PIN;
    PORTC.DIRSET = MOTOR_ENABLE_PIN;

    // PC1 is an external active-low fault input with an internal pull-up.
    PORTC.DIRCLR = FAULT_INPUT_PIN;
    PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
}

/**
 * Initializes the MCU clock and application GPIO.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void hardware_init(void)
{
    hardware_clock_init();
    hardware_gpio_init();
}

/**
 * Sets the status LED output.
 *
 * Args:
 *     enabled: True to turn the status LED on; false to turn it off.
 *
 * Returns:
 *     None.
 */
void hardware_status_led_set(bool enabled)
{
    if (enabled)
    {
        PORTF.OUTCLR = STATUS_LED_PIN;
    }
    else
    {
        PORTF.OUTSET = STATUS_LED_PIN;
    }
}

/**
 * Toggles the status LED output.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void hardware_status_led_toggle(void)
{
    PORTF.OUTTGL = STATUS_LED_PIN;
}

/**
 * Sets the demonstration motor-enable output.
 *
 * Args:
 *     enabled: True to enable the motor output; false to disable it.
 *
 * Returns:
 *     None.
 */
void hardware_motor_set(bool enabled)
{
    if (enabled)
    {
        PORTC.OUTSET = MOTOR_ENABLE_PIN;
    }
    else
    {
        PORTC.OUTCLR = MOTOR_ENABLE_PIN;
    }
}

/**
 * Reads the active-low user button input.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     True when the button is pressed; otherwise false.
 */
bool hardware_button_is_pressed(void)
{
    return ((PORTF.IN & BUTTON_PIN) == 0U);
}

/**
 * Reads the active-low external fault input.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     True when the fault input is active; otherwise false.
 */
bool hardware_fault_is_active(void)
{
    return ((PORTC.IN & FAULT_INPUT_PIN) == 0U);
}
