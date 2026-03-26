/**
 * @file    fsm.h
 * @brief   Generic Finite State Machine (FSM) engine.
 *
 * This module implements a table-driven FSM framework that is completely
 * independent of any specific application or hardware.  Any state machine
 * can be built on top of it by:
 *
 *   1. Defining an enumeration of states  (fsm_state_t  values).
 *   2. Defining an enumeration of events  (fsm_event_t  values).
 *   3. Building a transition table        (array of fsm_transition_t).
 *   4. Optionally providing entry/exit actions per state.
 *
 * -------------------------------------------------------------------------
 * Finite State Machine fundamentals
 * -------------------------------------------------------------------------
 *  A FSM is a computational model defined by:
 *    - A finite set of STATES, exactly one of which is "current" at any time.
 *    - A finite set of EVENTS (inputs / stimuli) that drive the machine.
 *    - A TRANSITION FUNCTION  f(state, event) -> next_state.
 *    - Optional ACTIONS executed when a transition fires or a state is
 *      entered / exited.
 *
 *  The table-driven approach stores the transition function as a flat array
 *  of (current_state, trigger_event, action, next_state) rows.
 *  Processing an event is an O(n) scan of the table – simple, transparent,
 *  and easy to extend.
 *
 * -------------------------------------------------------------------------
 * Author  : FSM Demo Project
 * Target  : AVR128DA48 Curiosity Nano
 * Toolchain: Atmel Studio 7 / avr-gcc
 * -------------------------------------------------------------------------
 */

#ifndef FSM_H_
#define FSM_H_

#include <stdint.h>
#include <stdbool.h>
#include "stddef.h"

/* =========================================================================
 * Type definitions
 * ========================================================================= */

/** @brief Opaque integer type used for both state and event identifiers.
 *
 *  Using a typedef keeps the engine independent of application enumerations.
 *  Cast application-specific enum values to fsm_id_t when filling tables.
 */
typedef uint8_t fsm_id_t;

/** @brief Sentinel value meaning "match any state" or "match any event".
 *
 *  Place FSM_ANY in the state or event field of a transition row to create a
 *  "wildcard" rule that fires regardless of the current state or the incoming
 *  event respectively.
 */
#define FSM_ANY  ((fsm_id_t)0xFF)

/**
 * @brief Pointer to a transition action function.
 *
 * An action is a void function that receives a pointer to the FSM instance so
 * it can read context data if needed.  Pass NULL for transitions that require
 * no side-effect.
 *
 * @param[in] ctx  Opaque pointer to application-specific context data.
 */
typedef void (*fsm_action_fn)(void *ctx);

/**
 * @brief One row of the transition table.
 *
 * The FSM engine scans the table in order.  The first row whose
 * (from_state, trigger) pair matches (current_state, incoming_event) fires:
 *   1. action() is called (if not NULL).
 *   2. current_state is updated to next_state.
 *
 * Fields
 * ------
 * from_state  – State in which this rule is active (or FSM_ANY).
 * trigger     – Event that activates this rule (or FSM_ANY).
 * action      – Function to call when the rule fires (or NULL).
 * next_state  – State to transition into after the action.
 */
typedef struct
{
    fsm_id_t       from_state;   /**< Guard: required current state          */
    fsm_id_t       trigger;      /**< Guard: required incoming event         */
    fsm_action_fn  action;       /**< Side-effect to execute (may be NULL)   */
    fsm_id_t       next_state;   /**< Destination state after this fires     */
} fsm_transition_t;

/**
 * @brief Optional per-state callback hooks.
 *
 * Provide an array of these (one per state, indexed by state id) to receive
 * entry and exit notifications.  Either pointer may be NULL.
 */
typedef struct
{
    fsm_action_fn on_entry;  /**< Called once when the state is entered  */
    fsm_action_fn on_exit;   /**< Called once just before the state exits */
} fsm_state_hooks_t;

/**
 * @brief FSM instance descriptor.
 *
 * Embed one of these inside your application context struct (or allocate it
 * statically).  Initialise it with fsm_init() before use.
 */
typedef struct
{
    fsm_id_t                   current_state;  /**< Active state id              */
    const fsm_transition_t    *table;          /**< Pointer to transition table  */
    uint8_t                    table_len;      /**< Number of rows in the table  */
    const fsm_state_hooks_t   *hooks;          /**< Per-state hooks (may be NULL)*/
    uint8_t                    num_states;     /**< Number of states (for hooks) */
    void                      *ctx;            /**< Opaque context pointer       */
} fsm_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * @brief Initialise an FSM instance.
 *
 * Must be called before any call to fsm_dispatch().
 *
 * @param[out] fsm        Pointer to the FSM instance to initialise.
 * @param[in]  init_state The state the machine should start in.
 * @param[in]  table      Transition table array.
 * @param[in]  table_len  Number of entries in @p table.
 * @param[in]  hooks      Per-state entry/exit hooks, or NULL if not needed.
 * @param[in]  num_states Number of states (length of @p hooks array).
 * @param[in]  ctx        Opaque context pointer forwarded to all actions.
 */
void fsm_init(fsm_t                   *fsm,
              fsm_id_t                 init_state,
              const fsm_transition_t  *table,
              uint8_t                  table_len,
              const fsm_state_hooks_t *hooks,
              uint8_t                  num_states,
              void                    *ctx);

/**
 * @brief Dispatch an event to the FSM.
 *
 * Scans the transition table for the first row where:
 *   (from_state == current_state  ||  from_state == FSM_ANY)  AND
 *   (trigger    == event          ||  trigger    == FSM_ANY)
 *
 * When a match is found:
 *   1. on_exit hook of the current state is called (if registered).
 *   2. The transition action is called (if not NULL).
 *   3. current_state is updated.
 *   4. on_entry hook of the new state is called (if registered).
 *
 * If no matching row is found the event is silently discarded.
 *
 * @param[in,out] fsm    The FSM instance.
 * @param[in]     event  The event to dispatch.
 * @return true  if a transition fired, false if the event was discarded.
 */
bool fsm_dispatch(fsm_t *fsm, fsm_id_t event);

/**
 * @brief Return the current state of an FSM instance.
 *
 * @param[in] fsm  The FSM instance.
 * @return The current state id.
 */
fsm_id_t fsm_state(const fsm_t *fsm);

#endif /* FSM_H_ */
