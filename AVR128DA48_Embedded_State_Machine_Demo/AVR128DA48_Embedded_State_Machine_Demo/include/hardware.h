#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdbool.h>

/**
 * Initializes the CPU clock and application GPIO pins.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void hardware_init(void);

/**
 * Controls the active-low status LED on PF5.
 *
 * Args:
 *     enabled: true to turn the LED on, false to turn it off.
 *
 * Returns:
 *     None.
 */
void hardware_status_led_set(bool enabled);

/**
 * Toggles the active-low status LED on PF5.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void hardware_status_led_toggle(void);

/**
 * Controls the demonstration motor-enable output on PC0.
 *
 * Args:
 *     enabled: true to drive PC0 high, false to drive it low.
 *
 * Returns:
 *     None.
 */
void hardware_motor_set(bool enabled);

/**
 * Reads the active-low push button on PF6.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     true while the push button is physically pressed, otherwise false.
 */
bool hardware_button_is_pressed(void);

/**
 * Reads the active-low fault input on PC1.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     true while the fault input is asserted, otherwise false.
 */
bool hardware_fault_is_active(void);

#endif
