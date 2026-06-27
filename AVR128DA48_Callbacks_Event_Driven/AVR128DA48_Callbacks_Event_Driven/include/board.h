#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>

/**
 * @brief Initializes the board GPIO used by the demonstration.
 */
void board_init(void);

/**
 * @brief Turns the user LED on.
 */
void board_led_on(void);

/**
 * @brief Turns the user LED off.
 */
void board_led_off(void);

/**
 * @brief Toggles the current user LED state.
 */
void board_led_toggle(void);

/**
 * @brief Reads the user button input.
 *
 * Returns:
 *     true while the active-low button is pressed, otherwise false.
 */
bool board_button_is_pressed(void);

#endif
