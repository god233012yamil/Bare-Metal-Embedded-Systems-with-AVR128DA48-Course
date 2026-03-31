/**
 * @file    evsys.h
 * @brief   Event System (EVSYS) configuration for the AVR128DA48.
 *
 * The Event System allows peripherals to communicate and trigger each other
 * without CPU intervention.  This module sets up two event channels:
 *
 *  Channel 0: AC0 output → ADC0 START event.
 *    When the Analog Comparator output toggles (threshold crossing), the ADC
 *    is triggered automatically, ensuring a sample is captured at the moment
 *    of the threshold crossing with minimal latency — independently of the
 *    cooperative scheduler tick.
 *
 *  Channel 1: ADC0 RESRDY → Software (informational).
 *    Demonstrates chaining; in a more complex design this could feed a DMA
 *    controller or a CCL gate.
 *
 * EVSYS is entirely hardware-managed after init; no ISR or CPU intervention
 * is required to move events along the channels.
 *
 * Hardware resources used:
 *  - EVSYS CHANNEL0
 *  - EVSYS CHANNEL1
 *  - EVSYS USERADC0START (user for ADC0 start trigger)
 *
 * Prerequisite: AC0 and ADC0 must be initialized with event-trigger support
 *               enabled (EVCTRL register) before EVSYS_Init() is called.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 */

#ifndef EVSYS_H_
#define EVSYS_H_

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Configures the Event System channels.
 *
 * Sets up:
 *  - CHANNEL0 generator: AC0_OUT.
 *  - CHANNEL0 user:      ADC0START (ADC0 start-of-conversion trigger).
 *
 * After this call, any AC0 output transition automatically triggers an ADC0
 * conversion without CPU involvement.
 *
 * Must be called after ADC_Init() and AC_Init() and before sei().
 */
void EVSYS_Init(void);

#endif /* EVSYS_H_ */
