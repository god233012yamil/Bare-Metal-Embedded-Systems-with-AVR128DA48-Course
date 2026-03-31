/**
 * @file    adc.h
 * @brief   ADC0 driver for the AVR128DA48.
 *
 * Provides a non-blocking, interrupt-driven ADC driver operating in
 * free-running single-ended mode.  A conversion is triggered by calling
 * ADC_StartConversion(); the result is captured in the RESRDY ISR and
 * stored in a volatile variable that the application reads via ADC_GetResult().
 *
 * Hardware resources used:
 *  - ADC0
 *  - PORTD PIN2 (AIN2) as analogue input — configure as input with
 *    input-disable bit set (no pull-up, no digital buffer).
 *  - VREF: internal 2.048 V reference.
 *
 * Layer contract:
 *  - This driver accesses hardware registers directly.
 *  - The application layer must never touch ADC0 registers after init.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initializes ADC0 and the VREF block.
 *
 * Configures:
 *  - VREF to supply 2.048 V internal reference to ADC0.
 *  - PD2 as analogue input (digital input buffer disabled).
 *  - ADC0 in 12-bit, single-ended, single-conversion mode.
 *  - RESRDY interrupt enabled.
 *
 * Global interrupts must be enabled by the caller (sei() in main).
 */
void ADC_Init(void);

/**
 * @brief Triggers a single ADC conversion.
 *
 * Call from the ADC task.  The result will be available via ADC_GetResult()
 * after the RESRDY interrupt fires (typically within a few µs at 1.5 MHz
 * ADC clock).  Calling this while a conversion is already in progress has no
 * effect (the STCONV bit write is ignored by hardware).
 */
void ADC_StartConversion(void);

/**
 * @brief Returns the most recent ADC conversion result.
 *
 * Safe to call from main context.  The returned value is a snapshot; a new
 * conversion may overwrite it asynchronously.
 *
 * @return 12-bit ADC result (0–4095).  Returns 0 before the first conversion
 *         completes.
 */
uint16_t ADC_GetResult(void);

/**
 * @brief Reports whether a new (unread) result is available.
 *
 * The flag is cleared by calling ADC_ClearResultFlag().
 *
 * @return true if a conversion completed since the last ADC_ClearResultFlag()
 *         call.
 */
bool ADC_IsResultReady(void);

/**
 * @brief Clears the "result ready" flag set by the RESRDY ISR.
 *
 * Call this after reading the result to avoid processing the same sample
 * twice.
 */
void ADC_ClearResultFlag(void);

#endif /* ADC_H_ */
