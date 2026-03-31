/**
 * @file    ac.h
 * @brief   Analog Comparator 0 (AC0) driver for the AVR128DA48.
 *
 * Configures AC0 to compare the analogue input (AINP0, PD2) against an
 * internally generated DAC reference voltage.  A rising-edge or falling-edge
 * interrupt signals the application layer that the input has crossed the
 * programmed threshold, enabling fast, event-driven reaction without polling.
 *
 * The AC output can also be routed to the Event System (EVSYS) to trigger
 * other peripherals autonomously — see evsys.h.
 *
 * Hardware resources used:
 *  - AC0
 *  - AINP0 → PD2 (shared with ADC AIN2; both can coexist because the digital
 *    input buffer is disabled on that pin).
 *  - Internal DAC reference (DACREF register inside AC0).
 *  - VREF: AC reference configured to VDD (set in ac.c).
 *
 * Layer contract:
 *  - Application must not access AC0 registers directly.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef AC_H_
#define AC_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes AC0 with the default threshold reference.
 *
 * Configures:
 *  - VREF to provide VDD as the AC DAC supply.
 *  - AC0 positive input: AINP0 (PD2).
 *  - AC0 negative input: internal DACREF.
 *  - DACREF to AC_DACREF_DEFAULT (defined in config.h).
 *  - Interrupt on both edges (BOTHEDGE) so the application knows when the
 *    signal crosses the threshold in either direction.
 *  - Small hysteresis to prevent chatter near the threshold.
 *  - AC0 enabled.
 *
 * Global interrupts must be enabled by the caller.
 */
void AC_Init(void);

/**
 * @brief Sets the AC threshold via the internal DAC reference.
 *
 * The DAC output voltage is:  V_ref = VDD * (dacref / 255)
 * At VDD = 3.3 V and dacref = 128: V_ref ≈ 1.65 V.
 *
 * @param[in] dacref  8-bit reference value (0–255).
 *                    0 → threshold at GND (always high output).
 *                    255 → threshold at VDD (always low output).
 */
void AC_SetThreshold(uint8_t dacref);

/**
 * @brief Returns the current AC output state.
 *
 * Reads the CMPSTATE bit from AC0.STATUS.  This bit reflects the live
 * comparator output state.
 *
 * @return true  if the positive input is above the negative reference
 *               (AC_CMPSTATE_bm is set in AC0.STATUS).
 * @return false if the positive input is at or below the reference.
 */
bool AC_GetOutput(void);

/**
 * @brief Reports whether the AC interrupt flag is set.
 *
 * Cleared by AC_ClearFlag().
 *
 * @return true if AC0 has fired since the last AC_ClearFlag() call.
 */
bool AC_IsFlagSet(void);

/**
 * @brief Clears the AC event flag tracked by the driver.
 *
 * Call this after processing an AC event in the application layer.
 */
void AC_ClearFlag(void);

#endif /* AC_H_ */
