/**
 * @file    tca.h
 * @brief   TCA0 single-slope PWM driver for the AVR128DA48.
 *
 * Configures Timer/Counter Type A (TCA0) in Single mode with single-slope
 * PWM waveform generation on Compare channel 0 (WO0).
 *
 * Hardware resources used:
 *  - TCA0 (single mode, WO0 output)
 *  - PORTMUX: TCA0 outputs routed to PORTD → WO0 on PD0.
 *  - PD0 configured as push-pull output.
 *
 * PWM parameters (set in config.h):
 *  - Clock: 24 MHz / DIV1 = 24 MHz TCA clock
 *  - Period register: 1023  → f_PWM ≈ 23.4 kHz
 *  - Duty cycle: 0–1023 (0 %–100 %)
 *
 * Layer contract:
 *  - Application must not access TCA0 or PORTMUX registers directly.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef TCA_H_
#define TCA_H_

#include <stdint.h>

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes TCA0 in single-slope PWM mode.
 *
 * Routes WO0 to PD0 via PORTMUX, sets PD0 as output, configures TCA0 with
 * the prescaler and period defined in config.h, enables WO0 output, and
 * starts the timer.  Duty cycle is initialized to 0 (output low).
 */
void TCA_Init(void);

/**
 * @brief Sets the PWM duty cycle.
 *
 * Writes the compare value to the CMP0BUF double-buffer register; the new
 * duty cycle takes effect at the next timer overflow (glitch-free update).
 *
 * @param[in] duty  Duty cycle count in the range [0, TCA0_PERIOD].  Values
 *                  larger than TCA0_PERIOD are clamped to TCA0_PERIOD (100 %).
 */
void TCA_SetDuty(uint16_t duty);

/**
 * @brief Returns the currently active duty cycle count.
 *
 * Reads CMP0 (the active, not the buffer, register).
 *
 * @return Current compare value (0–TCA0_PERIOD).
 */
uint16_t TCA_GetDuty(void);

/**
 * @brief Enables the PWM output (WO0).
 *
 * Sets the CMP0EN bit in TCA0.CTRLB without disturbing other settings.
 */
void TCA_EnableOutput(void);

/**
 * @brief Disables the PWM output (WO0) and forces PD0 low.
 *
 * Clears the CMP0EN bit in TCA0.CTRLB.
 */
void TCA_DisableOutput(void);

#endif /* TCA_H_ */
