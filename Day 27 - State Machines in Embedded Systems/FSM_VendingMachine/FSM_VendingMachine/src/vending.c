/**
 * @file    vending.c
 * @brief   Vending Machine FSM - application layer implementation.
 *
 * This file contains:
 *   - Transition table  : the complete FSM logic expressed as data
 *   - Action functions  : side-effects executed on each transition
 *   - State hooks       : on_entry / on_exit callbacks per state
 *   - LED update logic  : per-state blink patterns driven by system tick
 *   - vending_init()    : wires everything together
 *   - vending_run()     : the single superloop tick function
 *
 * Design philosophy
 * -----------------
 * All FSM logic is data, not code.  The transition table is a plain C array
 * of {from_state, event, action_fn, next_state} rows.  Adding a new
 * transition means adding one row to the table - no switch statements to
 * modify, no state functions to hunt down.
 *
 * The generic engine in fsm.c does the scanning; this file only defines
 * WHAT the machine does, not HOW the engine works.
 *
 * Author    : FSM Demo Project
 * Target    : AVR128DA48 Curiosity Nano
 * Toolchain : Atmel Studio 7 / avr-gcc
 */

#include <stddef.h>   /* NULL */
#include "vending.h"
#include "hal.h"
#include "fsm.h"

/* =========================================================================
 * Forward declarations of action functions
 *
 * Each action is a small function called by the FSM engine when the
 * matching transition row fires.  They receive the opaque context pointer
 * and cast it to vend_ctx_t* to access application state.
 * ========================================================================= */
static void act_insert_coin_partial (void *ctx);
static void act_insert_coin_full    (void *ctx);
static void act_add_coin_full       (void *ctx);
static void act_dispense            (void *ctx);
static void act_cancel_from_credit  (void *ctx);
static void act_cancel_from_full    (void *ctx);
static void act_fault               (void *ctx);
static void act_service_reset       (void *ctx);
static void act_dispense_complete   (void *ctx);

/* =========================================================================
 * Forward declarations of state-hook functions
 * ========================================================================= */
static void on_entry_idle        (void *ctx);
static void on_entry_has_credit  (void *ctx);
static void on_entry_full_credit (void *ctx);
static void on_entry_dispensing  (void *ctx);
static void on_entry_fault       (void *ctx);

/* =========================================================================
 * Transition table
 * =========================================================================
 *
 * This is the heart of the FSM.  Read each row as:
 *   "When in state FROM, and event TRIGGER occurs,
 *    execute ACTION and move to state NEXT."
 *
 * Columns:
 *   from_state         trigger        action                   next_state
 * -----------------------------------------------------------------------
 * The table is scanned top-to-bottom; the FIRST matching row fires.
 * Using FSM_ANY in either column creates a wildcard that matches any state
 * or any event respectively.
 */
static const fsm_transition_t vend_table[] =
{
    /* -----------------------------------------------------------------
     * From IDLE
     * ----------------------------------------------------------------- */

    /*  First coin inserted while idle -> move to HAS_CREDIT             */
    { VEND_ST_IDLE,        EVNT_COIN,     act_insert_coin_partial, VEND_ST_HAS_CREDIT  },

    /* -----------------------------------------------------------------
     * From HAS_CREDIT  (25 cents in machine, not enough to vend)
     * ----------------------------------------------------------------- */

    /*  Second coin -> enough credit, move to FULL_CREDIT                */
    { VEND_ST_HAS_CREDIT,  EVNT_COIN,     act_insert_coin_full,    VEND_ST_FULL_CREDIT },

    /*  Customer cancels or idle timeout fires -> refund, back to IDLE   */
    { VEND_ST_HAS_CREDIT,  EVNT_CANCEL,   act_cancel_from_credit,  VEND_ST_IDLE        },

    /* -----------------------------------------------------------------
     * From FULL_CREDIT  (50+ cents, ready to vend)
     * ----------------------------------------------------------------- */

    /*  Customer selects product -> start dispensing                     */
    { VEND_ST_FULL_CREDIT, EVNT_DISPENSE, act_dispense,            VEND_ST_DISPENSING  },

    /*  Customer cancels (changed their mind) -> refund, back to IDLE    */
    { VEND_ST_FULL_CREDIT, EVNT_CANCEL,   act_cancel_from_full,    VEND_ST_IDLE        },

    /*  Extra coin while at full credit (overpay) -> stay, log the coin  */
    { VEND_ST_FULL_CREDIT, EVNT_COIN,     act_add_coin_full,       VEND_ST_FULL_CREDIT },

    /* -----------------------------------------------------------------
     * From DISPENSING  (product being vended, LED fast-blinks)
     * ----------------------------------------------------------------- */

    /*  Internal timeout event -> dispense done, reset to IDLE           */
    { VEND_ST_DISPENSING,  EVNT_RESET,    act_dispense_complete,   VEND_ST_IDLE        },

    /* -----------------------------------------------------------------
     * Fault: can be triggered from ANY state
     * ----------------------------------------------------------------- */

    /*  ADC threshold exceeded -> enter FAULT from wherever we are       */
    { FSM_ANY,             EVNT_FAULT,    act_fault,               VEND_ST_FAULT       },

    /* -----------------------------------------------------------------
     * From FAULT  (hardware fault - LED shows SOS)
     * ----------------------------------------------------------------- */

    /*  Technician presses SERVICE -> recover, return to IDLE            */
    { VEND_ST_FAULT,       EVNT_SERVICE,  act_service_reset,       VEND_ST_IDLE        },
};

/** @brief Number of rows in the transition table. */
#define VEND_TABLE_LEN  ( (uint8_t)(sizeof(vend_table) / sizeof(vend_table[0])) )

/* =========================================================================
 * State hooks table
 *
 * Indexed by vend_state_t value.  on_entry is called when the FSM engine
 * enters a state; on_exit is called just before it leaves.
 * NULL means "no action needed".
 * ========================================================================= */
static const fsm_state_hooks_t vend_hooks[VEND_ST_COUNT] =
{
    /* VEND_ST_IDLE        */ { on_entry_idle,        NULL },
    /* VEND_ST_HAS_CREDIT  */ { on_entry_has_credit,  NULL },
    /* VEND_ST_FULL_CREDIT */ { on_entry_full_credit, NULL },
    /* VEND_ST_DISPENSING  */ { on_entry_dispensing,  NULL },
    /* VEND_ST_FAULT       */ { on_entry_fault,       NULL },
};

/* =========================================================================
 * Private helper: UART log helper
 * ========================================================================= */

/**
 * @brief Print a labelled key=value pair over UART.
 *
 * Output format:  "  label: value\r\n"
 *
 * @param[in] label  Descriptive prefix string.
 * @param[in] value  16-bit unsigned integer to print.
 */
static void log_kv(const char *label, uint16_t value)
{
    hal_uart_puts("  ");
    hal_uart_puts(label);
    hal_uart_puts(": ");
    hal_uart_put_u16(value);
    hal_uart_puts("\r\n");
}

/**
 * @brief Log a state transition over UART.
 *
 * Output example:
 *   [FSM] EVENT: COIN  |  IDLE -> HAS_CREDIT
 *
 * @param[in] event      The event that fired.
 * @param[in] from_state State before the transition.
 * @param[in] to_state   State after the transition.
 */
static void log_transition(vend_event_t event,
                            vend_state_t from_state,
                            vend_state_t to_state)
{
    hal_uart_puts("[FSM] EVENT: ");
    hal_uart_puts(vending_event_name((fsm_id_t)event));
    hal_uart_puts("  |  ");
    hal_uart_puts(vending_state_name((fsm_id_t)from_state));
    hal_uart_puts(" -> ");
    hal_uart_puts(vending_state_name((fsm_id_t)to_state));
    hal_uart_puts("\r\n");
}

/* =========================================================================
 * Private helper: event dispatch wrapper
 *
 * Wraps fsm_dispatch() and emits a UART log line before and after so that
 * every transition is visible on a serial terminal.
 * ========================================================================= */

/**
 * @brief Dispatch an event and log the transition over UART.
 *
 * @param[in,out] ctx    Application context (contains embedded fsm_t).
 * @param[in]     event  Event to dispatch.
 */
static void dispatch(vend_ctx_t *ctx, vend_event_t event)
{
    vend_state_t before = (vend_state_t)fsm_state(&ctx->fsm);

    bool fired = fsm_dispatch(&ctx->fsm, (fsm_id_t)event);

    if (fired)
    {
        vend_state_t after = (vend_state_t)fsm_state(&ctx->fsm);
        log_transition(event, before, after);
    }
    /* Silently discard events that have no matching rule (fired == false) */
}

/* =========================================================================
 * Action functions
 *
 * Each action is called by the FSM engine when the matching row fires.
 * Actions should be short: update context data, output to hardware, log.
 * They must NOT call dispatch() recursively - raise a flag instead and
 * handle it in vending_run() on the next iteration.
 * ========================================================================= */

/**
 * @brief Action: first coin inserted (partial credit).
 *
 * Adds VEND_COIN_VALUE to the credit counter and prints the new balance.
 * Resets the activity timestamp so the idle timeout starts from now.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_insert_coin_partial(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    c->credit_cents += VEND_COIN_VALUE;
    c->activity_ts   = hal_tick_ms();  /* reset idle timeout */

    hal_uart_puts("[ACT] Coin inserted (+");
    hal_uart_put_u16(VEND_COIN_VALUE);
    hal_uart_puts(" c)\r\n");
    log_kv("Credit", c->credit_cents);
    hal_uart_puts("[ACT] Insert one more coin to vend.\r\n");
}

/**
 * @brief Action: second coin inserted (credit now full).
 *
 * Adds VEND_COIN_VALUE to credit, which now meets or exceeds VEND_PRICE.
 * Prompts the user to press DISPENSE.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_insert_coin_full(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    c->credit_cents += VEND_COIN_VALUE;
    c->activity_ts   = hal_tick_ms();

    hal_uart_puts("[ACT] Coin inserted (+");
    hal_uart_put_u16(VEND_COIN_VALUE);
    hal_uart_puts(" c)\r\n");
    log_kv("Credit", c->credit_cents);
    hal_uart_puts("[ACT] Full credit! Press DISPENSE to vend.\r\n");
}

/**
 * @brief Action: extra coin added while already at full credit.
 *
 * Adds the coin to the credit register (will be refunded on cancel or
 * deducted on dispense).  Logs the overpayment.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_add_coin_full(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    c->credit_cents += VEND_COIN_VALUE;
    c->activity_ts   = hal_tick_ms();

    hal_uart_puts("[ACT] Extra coin accepted.\r\n");
    log_kv("Credit", c->credit_cents);
}

/**
 * @brief Action: dispense product.
 *
 * Deducts VEND_PRICE from credit, logs the change, and records the
 * dispense start timestamp for the auto-reset timeout.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_dispense(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    c->credit_cents  -= VEND_PRICE;   /* deduct item price */
    c->dispense_ts    = hal_tick_ms(); /* start dispense timer */

    hal_uart_puts("[ACT] DISPENSING product!\r\n");
    log_kv("Price paid", VEND_PRICE);
    log_kv("Change due", c->credit_cents);
}

/**
 * @brief Action: cancel from HAS_CREDIT state, refund credit.
 *
 * Resets credit to zero and informs the customer via UART.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_cancel_from_credit(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;

    hal_uart_puts("[ACT] Cancelled. Refunding ");
    hal_uart_put_u16(c->credit_cents);
    hal_uart_puts(" cents.\r\n");

    c->credit_cents = 0;
}

/**
 * @brief Action: cancel from FULL_CREDIT state, refund credit.
 *
 * Same as act_cancel_from_credit but called from the FULL_CREDIT state
 * (kept separate so messages can differ if desired in the future).
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_cancel_from_full(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;

    hal_uart_puts("[ACT] Selection cancelled. Refunding ");
    hal_uart_put_u16(c->credit_cents);
    hal_uart_puts(" cents.\r\n");

    c->credit_cents = 0;
}

/**
 * @brief Action: hardware fault detected.
 *
 * Logs the ADC value that triggered the fault.  The machine is now locked
 * until a SERVICE event is raised.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_fault(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;

    hal_uart_puts("[ACT] *** FAULT DETECTED ***\r\n");
    log_kv("ADC value", c->adc_value);
    hal_uart_puts("[ACT] Machine locked. Press SW0+DISPENSE to service.\r\n");
}

/**
 * @brief Action: service technician reset.
 *
 * Clears all credit (any coins still held are considered forfeit during
 * a service event), resets SOS blink step, and returns machine to IDLE.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_service_reset(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;

    c->credit_cents = 0;
    c->sos_step     = 0;

    hal_uart_puts("[ACT] Service reset acknowledged. Machine cleared.\r\n");
}

/**
 * @brief Action: dispense cycle complete (auto-reset).
 *
 * Called when the VEND_DISPENSE_TIME_MS timeout fires in the DISPENSING
 * state.  Logs any change due to the customer.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void act_dispense_complete(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;

    hal_uart_puts("[ACT] Dispense complete.\r\n");
    if (c->credit_cents > 0)
    {
        log_kv("Change returned", c->credit_cents);
    }
    c->credit_cents = 0;
}

/* =========================================================================
 * State entry hooks
 *
 * Called by the FSM engine immediately after current_state is updated.
 * Use them for one-shot actions: printing a banner, setting LED state,
 * recording timestamps.
 * ========================================================================= */

/**
 * @brief Entry hook: IDLE state.
 *
 * Turns the LED off and prints the IDLE banner.
 *
 * @param[in] ctx  Application context pointer (unused here).
 */
static void on_entry_idle(void *ctx)
{
    (void)ctx;  /* unused in this hook */
    hal_led_off();
    hal_uart_puts(">>> STATE: IDLE  (LED off - waiting for coin)\r\n");
}

/**
 * @brief Entry hook: HAS_CREDIT state.
 *
 * Turns the LED on (steady) and reminds the user to insert another coin.
 *
 * @param[in] ctx  Application context pointer.
 */
static void on_entry_has_credit(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    hal_led_on();
    c->activity_ts = hal_tick_ms();   /* start idle timeout from now */
    hal_uart_puts(">>> STATE: HAS_CREDIT  (LED on - insert one more coin)\r\n");
}

/**
 * @brief Entry hook: FULL_CREDIT state.
 *
 * Resets the LED blink timer so the blink pattern starts cleanly.
 *
 * @param[in] ctx  Application context pointer.
 */
static void on_entry_full_credit(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    hal_led_off();
    c->led_ts      = hal_tick_ms();   /* start LED blink timer */
    c->activity_ts = hal_tick_ms();   /* restart idle timeout */
    hal_uart_puts(">>> STATE: FULL_CREDIT  (LED blinking - press DISPENSE)\r\n");
}

/**
 * @brief Entry hook: DISPENSING state.
 *
 * Resets the LED fast-blink timer and logs dispense start.
 *
 * @param[in] ctx  Application context pointer.
 */
static void on_entry_dispensing(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    hal_led_off();
    c->led_ts      = hal_tick_ms();   /* start LED fast-blink timer */
    c->dispense_ts = hal_tick_ms();   /* start dispense timeout */
    hal_uart_puts(">>> STATE: DISPENSING  (LED fast-blink - vending...)\r\n");
}

/**
 * @brief Entry hook: FAULT state.
 *
 * Turns off the LED (SOS pattern is handled in vending_run) and logs.
 *
 * @param[in] ctx  Application context pointer.
 */
static void on_entry_fault(void *ctx)
{
    vend_ctx_t *c = (vend_ctx_t *)ctx;
    hal_led_off();
    c->led_ts  = hal_tick_ms();   /* start SOS timer */
    c->sos_step = 0;
    hal_uart_puts(">>> STATE: FAULT  (LED SOS pattern - service required)\r\n");
}

/* =========================================================================
 * LED pattern update functions
 *
 * These are called every iteration of vending_run().  They use non-blocking
 * elapsed-time checks (never delay()) against the system tick.
 * ========================================================================= */

/*
 * SOS timing parameters (milliseconds).
 * Morse: 3 short (dit), 3 long (dah), 3 short (dit), pause.
 */
#define SOS_DIT_MS   150u   /**< Short flash on-time  */
#define SOS_DAH_MS   450u   /**< Long  flash on-time  */
#define SOS_GAP_MS   150u   /**< Gap between elements */
#define SOS_PAUSE_MS 800u   /**< Pause between SOS repeats */

/**
 * @brief SOS step durations in milliseconds.
 *
 * Encodes the full SOS pattern as a sequence of on/off times.
 * Odd indexes = LED on,  Even indexes = LED off (gap or pause).
 *
 * Pattern: S(dit dit dit) O(dah dah dah) S(dit dit dit) [pause]
 *
 *  Step:  0    1    2    3    4    5    6    7    8    9   10   11   12
 *  State: OFF  ON   OFF  ON   OFF  ON   OFF  ON   OFF  ON  OFF  ON   OFF
 *  Time:  GAP  DIT  GAP  DIT  GAP  DIT  GAP  DAH  GAP  DAH GAP  DAH  PAUSE...
 *
 * Steps 13-18 repeat the final S (dit dit dit), then step 19 is the long pause.
 */
static const uint16_t sos_timings[] =
{
    /*  0 off */ SOS_GAP_MS,
    /*  1 on  */ SOS_DIT_MS,   /* S - dit 1 */
    /*  2 off */ SOS_GAP_MS,
    /*  3 on  */ SOS_DIT_MS,   /* S - dit 2 */
    /*  4 off */ SOS_GAP_MS,
    /*  5 on  */ SOS_DIT_MS,   /* S - dit 3 */
    /*  6 off */ SOS_GAP_MS,
    /*  7 on  */ SOS_DAH_MS,   /* O - dah 1 */
    /*  8 off */ SOS_GAP_MS,
    /*  9 on  */ SOS_DAH_MS,   /* O - dah 2 */
    /* 10 off */ SOS_GAP_MS,
    /* 11 on  */ SOS_DAH_MS,   /* O - dah 3 */
    /* 12 off */ SOS_GAP_MS,
    /* 13 on  */ SOS_DIT_MS,   /* S - dit 1 */
    /* 14 off */ SOS_GAP_MS,
    /* 15 on  */ SOS_DIT_MS,   /* S - dit 2 */
    /* 16 off */ SOS_GAP_MS,
    /* 17 on  */ SOS_DIT_MS,   /* S - dit 3 */
    /* 18 off */ SOS_PAUSE_MS, /* long gap before repeat */
};

#define SOS_STEPS  ( (uint8_t)(sizeof(sos_timings) / sizeof(sos_timings[0])) )

/**
 * @brief Update the SOS LED pattern for the FAULT state.
 *
 * Non-blocking: checks whether sos_timings[sos_step] ms have elapsed since
 * the last step change.  Advances to the next step and sets the LED
 * accordingly.  Odd steps = LED on, even steps = LED off.
 *
 * @param[in,out] ctx  Application context pointer.
 */
static void led_update_sos(vend_ctx_t *ctx)
{
    if (hal_elapsed_ms(ctx->led_ts) >= sos_timings[ctx->sos_step])
    {
        ctx->sos_step = (ctx->sos_step + 1) % SOS_STEPS;
        ctx->led_ts   = hal_tick_ms();

        /* Odd steps = LED ON, even steps = LED OFF */
        if (ctx->sos_step & 0x01)
        {
            hal_led_on();
        }
        else
        {
            hal_led_off();
        }
    }
}

/**
 * @brief Update LED blink at a given period for FULL_CREDIT or DISPENSING.
 *
 * Toggles the LED each time half_period_ms have elapsed.
 *
 * @param[in,out] ctx           Application context pointer.
 * @param[in]     half_period_ms Half the desired blink period in ms.
 */
static void led_update_blink(vend_ctx_t *ctx, uint16_t half_period_ms)
{
    if (hal_elapsed_ms(ctx->led_ts) >= half_period_ms)
    {
        ctx->led_ts = hal_tick_ms();
        hal_led_toggle();
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initialise the vending machine FSM and context.
 *
 * Zeroes the context struct, wires the FSM engine to the transition table
 * and hook array, then calls fsm_init() to set the initial state (IDLE)
 * and fire its on_entry hook.  Prints a boot banner over UART.
 *
 * @param[out] ctx  Uninitialised application context; fully populated here.
 */
void vending_init(vend_ctx_t *ctx)
{
    /* Zero everything first so no field has garbage */
    ctx->credit_cents = 0;
    ctx->activity_ts  = 0;
    ctx->dispense_ts  = 0;
    ctx->led_ts       = 0;
    ctx->sos_step     = 0;
    ctx->adc_value    = 0;

    /* Boot banner */
    hal_uart_puts("\r\n");
    hal_uart_puts("========================================\r\n");
    hal_uart_puts("  FSM Vending Machine Demo\r\n");
    hal_uart_puts("  AVR128DA48 Curiosity Nano\r\n");
    hal_uart_puts("----------------------------------------\r\n");
    hal_uart_puts("  Buttons:\r\n");
    hal_uart_puts("    PA2 (COIN)     - Insert 25c coin\r\n");
    hal_uart_puts("    PA3 (DISPENSE) - Select product\r\n");
    hal_uart_puts("    PC7 (SW0)      - Cancel / refund\r\n");
    hal_uart_puts("    SW0+DISPENSE   - Service reset\r\n");
    hal_uart_puts("  ADC on PD3: raise voltage > 2.5V\r\n");
    hal_uart_puts("             to simulate fault.\r\n");
    hal_uart_puts("========================================\r\n\r\n");

    /*
     * Initialise the FSM engine.
     *  - Initial state : VEND_ST_IDLE
     *  - Table         : vend_table[]  (defined above)
     *  - Hooks         : vend_hooks[]  (entry/exit per state)
     *  - Context       : ctx           (passed to every action/hook)
     */
    fsm_init(&ctx->fsm,
             (fsm_id_t)VEND_ST_IDLE,
             vend_table,
             VEND_TABLE_LEN,
             vend_hooks,
             (uint8_t)VEND_ST_COUNT,
             ctx);
}

/**
 * @brief Run one superloop iteration of the vending machine.
 *
 * Called repeatedly from main().  The function is structured in five
 * clearly separated phases:
 *
 *  Phase 1 – Button polling
 *    Each button is checked with hal_btn_pressed() (debounced, edge-only).
 *    A matching event is dispatched to the FSM.
 *    SW0 + DISPENSE pressed together generates EVNT_SERVICE (service reset).
 *
 *  Phase 2 – ADC fault detection
 *    hal_adc_ready() is checked and a new conversion is triggered if needed.
 *    If the result exceeds VEND_ADC_FAULT_THRESH, EVNT_FAULT is dispatched.
 *
 *  Phase 3 – Idle timeout
 *    In HAS_CREDIT and FULL_CREDIT, if no activity has occurred for
 *    VEND_IDLE_TIMEOUT_MS ms, EVNT_CANCEL is automatically dispatched.
 *
 *  Phase 4 – Dispense timeout
 *    In DISPENSING, after VEND_DISPENSE_TIME_MS ms, EVNT_RESET is
 *    automatically dispatched to complete the vend cycle.
 *
 *  Phase 5 – LED update
 *    The correct LED animation function is called based on current state.
 *
 * @param[in,out] ctx  Application context pointer.
 */
void vending_run(vend_ctx_t *ctx)
{
    vend_state_t state = (vend_state_t)fsm_state(&ctx->fsm);

    /* ------------------------------------------------------------------
     * Phase 1: Button polling
     *
     * Read all buttons.  If both SW0 and DISPENSE are pressed in the same
     * polling cycle, treat it as a SERVICE event (technician key combo).
     * Otherwise handle each button independently.
     * ------------------------------------------------------------------ */
    bool sw0      = hal_btn_pressed(HAL_BTN_SW0);
    bool coin     = hal_btn_pressed(HAL_BTN_COIN);
    bool dispense = hal_btn_pressed(HAL_BTN_DISPENSE);

    if (sw0 && dispense)
    {
        /* Both pressed simultaneously -> service key combo */
        dispatch(ctx, EVNT_SERVICE);
    }
    else
    {
        if (coin)     dispatch(ctx, EVNT_COIN);
        if (dispense) dispatch(ctx, EVNT_DISPENSE);
        if (sw0)      dispatch(ctx, EVNT_CANCEL);
    }

    /* Update state after possible transitions above */
    state = (vend_state_t)fsm_state(&ctx->fsm);

    /* ------------------------------------------------------------------
     * Phase 2: ADC fault detection
     *
     * If no conversion is in progress, start one.  When a result arrives,
     * store it in context (for logging in act_fault) and check the threshold.
     * ------------------------------------------------------------------ */
    if (hal_adc_ready())
    {
        ctx->adc_value = hal_adc_read();

        if (ctx->adc_value >= VEND_ADC_FAULT_THRESH)
        {
            /* Only raise a fault if not already in FAULT state            */
            if (state != VEND_ST_FAULT)
            {
                dispatch(ctx, EVNT_FAULT);
                state = (vend_state_t)fsm_state(&ctx->fsm);
            }
        }
        else
        {
            /* Kick off the next conversion immediately                    */
            hal_adc_start();
        }
    }
    else
    {
        /* No result pending - start a conversion if ADC is not busy      */
        hal_adc_start();
    }

    /* ------------------------------------------------------------------
     * Phase 3: Idle timeout (HAS_CREDIT and FULL_CREDIT only)
     *
     * If the customer inserts a coin but does nothing for VEND_IDLE_TIMEOUT_MS
     * milliseconds, automatically cancel and refund.
     * ------------------------------------------------------------------ */
    if ((state == VEND_ST_HAS_CREDIT) || (state == VEND_ST_FULL_CREDIT))
    {
        if (hal_elapsed_ms(ctx->activity_ts) >= VEND_IDLE_TIMEOUT_MS)
        {
            hal_uart_puts("[TMR] Idle timeout - auto-cancel.\r\n");
            dispatch(ctx, EVNT_CANCEL);
            state = (vend_state_t)fsm_state(&ctx->fsm);
        }
    }

    /* ------------------------------------------------------------------
     * Phase 4: Dispense timeout (DISPENSING only)
     *
     * After VEND_DISPENSE_TIME_MS ms in the DISPENSING state the machine
     * auto-resets to IDLE via an internal EVNT_RESET event.
     * ------------------------------------------------------------------ */
    if (state == VEND_ST_DISPENSING)
    {
        if (hal_elapsed_ms(ctx->dispense_ts) >= VEND_DISPENSE_TIME_MS)
        {
            hal_uart_puts("[TMR] Dispense timeout - resetting.\r\n");
            dispatch(ctx, EVNT_RESET);
            state = (vend_state_t)fsm_state(&ctx->fsm);
        }
    }

    /* ------------------------------------------------------------------
     * Phase 5: LED update
     *
     * Driven entirely by non-blocking elapsed-time checks so the superloop
     * is never stalled.
     * ------------------------------------------------------------------ */
    switch (state)
    {
        case VEND_ST_IDLE:
            /* LED is permanently off - nothing to update periodically     */
            break;

        case VEND_ST_HAS_CREDIT:
            /* LED is permanently on - nothing to update periodically      */
            break;

        case VEND_ST_FULL_CREDIT:
            /* Slow blink: toggle every 500 ms                             */
            led_update_blink(ctx, 500u);
            break;

        case VEND_ST_DISPENSING:
            /* Fast blink: toggle every 100 ms                             */
            led_update_blink(ctx, 100u);
            break;

        case VEND_ST_FAULT:
            /* SOS Morse pattern                                            */
            led_update_sos(ctx);
            break;

        default:
            break;
    }
}

/* =========================================================================
 * Debug name helpers
 * ========================================================================= */

/**
 * @brief Return a constant string name for a state.
 *
 * @param[in] state  vend_state_t value cast to fsm_id_t.
 * @return    Pointer to a constant string literal.
 */
const char *vending_state_name(fsm_id_t state)
{
    switch ((vend_state_t)state)
    {
        case VEND_ST_IDLE:        return "IDLE";
        case VEND_ST_HAS_CREDIT:  return "HAS_CREDIT";
        case VEND_ST_FULL_CREDIT: return "FULL_CREDIT";
        case VEND_ST_DISPENSING:  return "DISPENSING";
        case VEND_ST_FAULT:       return "FAULT";
        default:                  return "UNKNOWN";
    }
}

/**
 * @brief Return a constant string name for an event.
 *
 * @param[in] event  vend_event_t value cast to fsm_id_t.
 * @return    Pointer to a constant string literal.
 */
const char *vending_event_name(fsm_id_t event)
{
    switch ((vend_event_t)event)
    {
        case EVNT_COIN:     return "COIN";
        case EVNT_DISPENSE: return "DISPENSE";
        case EVNT_CANCEL:   return "CANCEL";
        case EVNT_FAULT:    return "FAULT";
        case EVNT_SERVICE:  return "SERVICE";
        case EVNT_RESET:    return "RESET";
        default:            return "UNKNOWN";
    }
}
