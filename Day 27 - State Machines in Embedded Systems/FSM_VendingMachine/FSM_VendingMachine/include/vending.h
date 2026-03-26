/**
 * @file    vending.h
 * @brief   Vending Machine application layer - FSM states, events, and API.
 *
 * =========================================================================
 * Application Overview
 * =========================================================================
 * This module implements a coin-operated vending machine controller using
 * the generic FSM engine (fsm.h / fsm.c).  It is a classic FSM teaching
 * example because it has:
 *   - Clearly named states with distinct LED behaviours
 *   - Multiple input events (buttons, ADC fault, timer)
 *   - Guard data (accumulated credit counter)
 *   - Real hardware side-effects (LED, UART debug log)
 *   - Both external events (button presses) and internal events (timeouts)
 *
 * =========================================================================
 * State Diagram
 * =========================================================================
 *
 *  Power-on
 *      |
 *      v
 *  +----------+   COIN(1st)    +-----------+   COIN(2nd)   +------------+
 *  |  IDLE    |--------------->| HAS_CREDIT|-------------->| FULL_CREDIT|
 *  |          |                |           |               |            |
 *  | LED: OFF |                | LED: ON   |               | LED: BLINK |
 *  +----------+                +-----------+               +------------+
 *      ^  ^                         |                            |
 *      |  |         CANCEL/timeout  |           CANCEL/timeout   |
 *      |  +-------------------------+----------------------------->
 *      |                                               DISPENSE  |
 *      |                                                          v
 *      |  RESET (auto)             +------------+       +------------+
 *      +<--------------------------|  DISPENSING|<------| FULL_CREDIT|
 *      |                           |            |       +------------+
 *      |                           | LED: FAST  |
 *      |                           |    BLINK   |
 *      |                           +------------+
 *      |
 *      |  FAULT event (ADC high)   +------------+
 *      +<--------------------------| FAULT      |
 *           SERVICE event          |            |
 *                                  | LED: SOS   |
 *                                  +------------+
 *
 * =========================================================================
 * Credit Model
 * =========================================================================
 *  Each COIN event adds VEND_COIN_VALUE (25) cents to the credit register.
 *  - 0 credit             -> IDLE
 *  - 1 coin  (25 cents)   -> HAS_CREDIT
 *  - 2+ coins (>= 50 c)   -> FULL_CREDIT (sufficient to vend)
 *  CANCEL from any credit state refunds all credit and returns to IDLE.
 *
 * =========================================================================
 * Timeouts (polled in the main loop)
 * =========================================================================
 *  - VEND_IDLE_TIMEOUT_MS : auto-CANCEL if no activity in HAS_CREDIT or
 *                           FULL_CREDIT (simulates anti-jam timer).
 *  - VEND_DISPENSE_TIME_MS: auto-RESET after dispensing completes.
 *
 * =========================================================================
 * Fault Detection
 * =========================================================================
 *  The ADC on PD3 (potentiometer) simulates a temperature/jam sensor.
 *  If the reading exceeds VEND_ADC_FAULT_THRESH, an EVNT_FAULT is raised.
 *  Recovery is triggered by pressing SW0 + DISPENSE simultaneously
 *  (simulated by the SERVICE event).
 *
 * =========================================================================
 * LED Behaviour Per State
 * =========================================================================
 *  IDLE        - LED permanently OFF
 *  HAS_CREDIT  - LED permanently ON
 *  FULL_CREDIT - LED slow blink (500 ms)
 *  DISPENSING  - LED fast blink (100 ms)
 *  FAULT       - LED SOS pattern (3 short, 3 long, 3 short)
 *
 * =========================================================================
 * Author    : FSM Demo Project
 * Target    : AVR128DA48 Curiosity Nano
 * Toolchain : Atmel Studio 7 / avr-gcc
 * =========================================================================
 */

#ifndef VENDING_H_
#define VENDING_H_

#include <stdint.h>
#include "fsm.h"

/* =========================================================================
 * Configuration constants
 * ========================================================================= */

/** @brief Price of one item in cents. Two coins required. */
#define VEND_PRICE              50u

/** @brief Value added per coin insert event, in cents. */
#define VEND_COIN_VALUE         25u

/**
 * @brief Idle timeout in milliseconds.
 *
 * If no button is pressed for this long while credit is held, the machine
 * auto-cancels and refunds, preventing credit from being stuck forever.
 */
#define VEND_IDLE_TIMEOUT_MS    10000u   /* 10 seconds */

/**
 * @brief Dispense animation duration in milliseconds.
 *
 * After a DISPENSE event the machine stays in DISPENSING state for this
 * long (LED fast-blinks to indicate activity) then auto-resets to IDLE.
 */
#define VEND_DISPENSE_TIME_MS   2000u    /* 2 seconds  */

/**
 * @brief ADC threshold for fault detection (12-bit, 0-4095).
 *
 * The potentiometer on PD3 simulates a temperature / jam sensor.
 * At VDD reference, this threshold corresponds to approximately 2.5 V:
 *   threshold = (2.5 / 3.3) * 4096 ≈ 3103
 */
#define VEND_ADC_FAULT_THRESH   3103u

/* =========================================================================
 * FSM state identifiers
 * ========================================================================= */

/**
 * @brief All states of the vending machine FSM.
 *
 * The numeric values serve as indices into the transition table and the
 * state-hooks array, so they must be consecutive starting from 0.
 */
typedef enum
{
    VEND_ST_IDLE        = 0,  /**< Machine idle, no credit held           */
    VEND_ST_HAS_CREDIT  = 1,  /**< One coin inserted (25 c), awaiting more */
    VEND_ST_FULL_CREDIT = 2,  /**< Sufficient credit (>= 50 c) to vend    */
    VEND_ST_DISPENSING  = 3,  /**< Product being dispensed (timed)        */
    VEND_ST_FAULT       = 4,  /**< Hardware fault detected (ADC high)     */
    VEND_ST_COUNT             /**< Total number of states - keep last     */
} vend_state_t;

/* =========================================================================
 * FSM event identifiers
 * ========================================================================= */

/**
 * @brief All events that can drive the vending machine FSM.
 *
 * Events are raised by:
 *   - Button presses  (hardware, debounced via HAL)
 *   - ADC threshold   (polled in main loop)
 *   - Timeout expiry  (polled in main loop using system tick)
 */
typedef enum
{
    EVNT_COIN     = 0,  /**< A coin was inserted (BTN_COIN pressed)         */
    EVNT_DISPENSE = 1,  /**< Product select button pressed (BTN_DISPENSE)   */
    EVNT_CANCEL   = 2,  /**< Cancel / refund pressed (BTN_SW0) or timeout   */
    EVNT_FAULT    = 3,  /**< ADC reading exceeded fault threshold           */
    EVNT_SERVICE  = 4,  /**< Service / reset: SW0 held during power-on      */
    EVNT_RESET    = 5,  /**< Internal: dispense complete, return to IDLE    */
    EVNT_COUNT          /**< Total number of events - keep last             */
} vend_event_t;

/* =========================================================================
 * Application context
 * ========================================================================= */

/**
 * @brief All runtime data for the vending machine application.
 *
 * This struct is passed as the opaque context pointer (void *ctx) to every
 * FSM action and hook.  Casting it back to vend_ctx_t* inside those
 * functions gives access to the shared state without globals.
 */
typedef struct
{
    fsm_t    fsm;           /**< Embedded FSM instance (must be first)      */
    uint8_t  credit_cents;  /**< Accumulated credit in cents                */
    uint32_t activity_ts;   /**< Tick snapshot for idle timeout             */
    uint32_t dispense_ts;   /**< Tick snapshot for dispense timeout         */
    uint32_t led_ts;        /**< Tick snapshot for LED blink timing         */
    uint8_t  sos_step;      /**< Current step in the SOS LED pattern        */
    uint16_t adc_value;     /**< Latest ADC sample                          */
} vend_ctx_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initialise the vending machine FSM and context.
 *
 * Populates the transition table and state-hook arrays, calls fsm_init(),
 * resets the credit counter, and prints a boot banner over UART.
 *
 * @param[out] ctx  Pointer to the application context to initialise.
 */
void vending_init(vend_ctx_t *ctx);

/**
 * @brief Run one iteration of the vending machine main loop.
 *
 * Must be called repeatedly from the superloop in main().  Each call:
 *   1. Polls all three buttons via HAL and raises the corresponding event.
 *   2. Polls the ADC and raises EVNT_FAULT if the threshold is exceeded.
 *   3. Checks the idle timeout and raises EVNT_CANCEL if expired.
 *   4. Checks the dispense timeout and raises EVNT_RESET if expired.
 *   5. Updates the LED pattern for the current state.
 *
 * @param[in,out] ctx  Pointer to the application context.
 */
void vending_run(vend_ctx_t *ctx);

/**
 * @brief Return a human-readable name for a vending machine state.
 *
 * Useful for UART debug logging.
 *
 * @param[in] state  A vend_state_t cast to fsm_id_t.
 * @return    Null-terminated constant string, e.g. "IDLE".
 */
const char *vending_state_name(fsm_id_t state);

/**
 * @brief Return a human-readable name for a vending machine event.
 *
 * @param[in] event  A vend_event_t cast to fsm_id_t.
 * @return    Null-terminated constant string, e.g. "COIN".
 */
const char *vending_event_name(fsm_id_t event);

#endif /* VENDING_H_ */
