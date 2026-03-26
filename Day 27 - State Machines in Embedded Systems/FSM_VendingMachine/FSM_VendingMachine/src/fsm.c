/**
 * @file    fsm.c
 * @brief   Generic table-driven Finite State Machine engine – implementation.
 *
 * See fsm.h for the full design description.
 *
 * Implementation notes
 * --------------------
 * - The engine is intentionally tiny: no dynamic memory, no RTOS dependency.
 * - All state is held in the fsm_t struct supplied by the caller.
 * - Entry/exit hooks and the action callback all receive the same opaque
 *   context pointer so application code can share data without globals.
 *
 * Author  : FSM Demo Project
 * Target  : AVR128DA48 Curiosity Nano
 * Toolchain: Atmel Studio 7 / avr-gcc
 */

#include "fsm.h"

/* =========================================================================
 * Public API – definitions
 * ========================================================================= */

/**
 * @brief Initialise an FSM instance.
 *
 * Stores the transition table, hook array and context pointer into the fsm_t
 * struct, sets the initial state, then fires the on_entry hook of the initial
 * state (if one is registered).
 */
void fsm_init(fsm_t                   *fsm,
              fsm_id_t                 init_state,
              const fsm_transition_t  *table,
              uint8_t                  table_len,
              const fsm_state_hooks_t *hooks,
              uint8_t                  num_states,
              void                    *ctx)
{
    fsm->current_state = init_state;
    fsm->table         = table;
    fsm->table_len     = table_len;
    fsm->hooks         = hooks;
    fsm->num_states    = num_states;
    fsm->ctx           = ctx;

    /* Fire entry hook for the initial state */
    if ((hooks != NULL) && (init_state < num_states) &&
        (hooks[init_state].on_entry != NULL))
    {
        hooks[init_state].on_entry(ctx);
    }
}

/**
 * @brief Dispatch an event to the FSM.
 *
 * Linear scan of the transition table.  The first matching row wins.
 * Matching rule:
 *   (row.from_state == FSM_ANY  ||  row.from_state == fsm->current_state)
 *   AND
 *   (row.trigger    == FSM_ANY  ||  row.trigger    == event)
 *
 * Transition sequence:
 *   1. on_exit  of current state  (if hooks != NULL and hook registered)
 *   2. row.action                 (if not NULL)
 *   3. current_state = row.next_state
 *   4. on_entry of new state      (if hooks != NULL and hook registered)
 *
 * Note: on_exit and on_entry are called even when the state does not change
 * (i.e. self-transitions are fully supported).
 */
bool fsm_dispatch(fsm_t *fsm, fsm_id_t event)
{
    uint8_t i;

    /* Scan transition table for a matching row */
    for (i = 0; i < fsm->table_len; i++)
    {
        const fsm_transition_t *row = &fsm->table[i];

        bool state_match = (row->from_state == FSM_ANY) ||
                           (row->from_state == fsm->current_state);

        bool event_match = (row->trigger == FSM_ANY) ||
                           (row->trigger == event);

        if (state_match && event_match)
        {
            /* ---- Step 1: call on_exit for the CURRENT state ----------- */
            if ((fsm->hooks != NULL) &&
                (fsm->current_state < fsm->num_states) &&
                (fsm->hooks[fsm->current_state].on_exit != NULL))
            {
                fsm->hooks[fsm->current_state].on_exit(fsm->ctx);
            }

            /* ---- Step 2: execute the transition action ---------------- */
            if (row->action != NULL)
            {
                row->action(fsm->ctx);
            }

            /* ---- Step 3: update current state ------------------------- */
            fsm->current_state = row->next_state;

            /* ---- Step 4: call on_entry for the NEW state -------------- */
            if ((fsm->hooks != NULL) &&
                (fsm->current_state < fsm->num_states) &&
                (fsm->hooks[fsm->current_state].on_entry != NULL))
            {
                fsm->hooks[fsm->current_state].on_entry(fsm->ctx);
            }

            return true; /* A transition fired */
        }
    }

    return false; /* No matching rule – event discarded */
}

/**
 * @brief Return the current state identifier.
 *
 * A thin accessor so callers do not need to dereference fsm_t directly.
 */
fsm_id_t fsm_state(const fsm_t *fsm)
{
    return fsm->current_state;
}
