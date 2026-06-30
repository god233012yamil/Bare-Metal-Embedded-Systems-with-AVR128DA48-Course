#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes TCB0 as a one-millisecond system tick source.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void system_time_init(void);

/**
 * Returns the current system time in milliseconds.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Milliseconds elapsed since initialization, with natural 32-bit wraparound.
 */
uint32_t system_time_get_ms(void);

/**
 * Tests whether a deadline has been reached using wraparound-safe arithmetic.
 *
 * Args:
 *     now_ms: Current system time in milliseconds.
 *     deadline_ms: Deadline to compare against.
 *
 * Returns:
 *     true when now_ms is at or after deadline_ms, otherwise false.
 */
bool system_time_deadline_reached(uint32_t now_ms, uint32_t deadline_ms);

#endif
