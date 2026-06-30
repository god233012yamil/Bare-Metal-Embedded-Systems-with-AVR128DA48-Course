#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes the non-blocking button debounce state machine.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     None.
 */
void button_init(uint32_t now_ms);

/**
 * Updates the button debounce state machine.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *
 * Returns:
 *     true once for each validated button press, otherwise false.
 */
bool button_update(uint32_t now_ms);

#endif
