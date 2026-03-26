/**
 * @file    main.c
 * @brief   FSM Vending Machine Demo - application entry point.
 *
 * =========================================================================
 * Project Summary
 * =========================================================================
 * This project demonstrates a Finite State Machine (FSM) in embedded C on
 * the AVR128DA48 Curiosity Nano board.
 *
 * The chosen application is a coin-operated VENDING MACHINE controller.
 * It is an ideal FSM teaching example because:
 *
 *   1. States are concrete and understandable (IDLE, HAS_CREDIT, etc.)
 *   2. Events come from multiple real sources (buttons, ADC, timers)
 *   3. Guard data (credit counter) shows how context enriches an FSM
 *   4. The machine has both external (user) and internal (timeout) events
 *   5. LED patterns give immediate visual feedback of state changes
 *   6. UART output provides a full transition log for learning/debugging
 *
 * =========================================================================
 * Software Architecture
 * =========================================================================
 *
 *   main.c          - Entry point, superloop, wires everything together
 *   vending.h/.c    - Application FSM: states, events, actions, hooks
 *   fsm.h/.c        - Generic reusable FSM engine (table-driven)
 *   hal.h/.c        - Hardware Abstraction Layer: GPIO, UART, ADC, tick
 *
 *   ???????????     ??????????????????     ?????????????
 *   ?  main.c ???????  vending.c     ???????  fsm.c    ?
 *   ?         ?     ?  (application) ?     ? (engine)  ?
 *   ???????????     ??????????????????     ?????????????
 *                            ?
 *                            ?
 *                   ??????????????????
 *                   ?    hal.c       ?
 *                   ?  (hardware)    ?
 *                   ??????????????????
 *
 * =========================================================================
 * Hardware Requirements
 * =========================================================================
 *   Board   : AVR128DA48 Curiosity Nano
 *   LED0    : PC6  (onboard, active LOW)
 *   SW0     : PC7  (onboard, active LOW - CANCEL / service combo)
 *   COIN    : PA2  (external button to GND with internal pull-up)
 *   DISPENSE: PA3  (external button to GND with internal pull-up)
 *   ADC in  : PD3  (potentiometer between VDD and GND)
 *   UART TX : PA0  (connect to USB-UART bridge, 9600 8N1)
 *
 * =========================================================================
 * How to use
 * =========================================================================
 *   1. Connect a USB-UART adapter to PA0 and open a terminal at 9600 8N1.
 *   2. Press PA2 (COIN)     once  -> IDLE transitions to HAS_CREDIT.
 *   3. Press PA2 (COIN)     again -> HAS_CREDIT transitions to FULL_CREDIT.
 *   4. Press PA3 (DISPENSE)       -> FULL_CREDIT transitions to DISPENSING.
 *   5. Wait 2 seconds             -> Auto-reset back to IDLE.
 *   6. Raise PD3 voltage > 2.5 V  -> Any state transitions to FAULT.
 *   7. Press PC7 + PA3 together   -> SERVICE event, return to IDLE.
 *   8. Press PC7 (SW0) alone      -> CANCEL from any credit state.
 *
 * =========================================================================
 * UART output example
 * =========================================================================
 *   ========================================
 *     FSM Vending Machine Demo
 *     AVR128DA48 Curiosity Nano
 *   ----------------------------------------
 *   >>> STATE: IDLE  (LED off - waiting for coin)
 *   [FSM] EVENT: COIN  |  IDLE -> HAS_CREDIT
 *   [ACT] Coin inserted (+25 c)
 *     Credit: 25
 *   >>> STATE: HAS_CREDIT  (LED on - insert one more coin)
 *   [FSM] EVENT: COIN  |  HAS_CREDIT -> FULL_CREDIT
 *   [ACT] Coin inserted (+25 c)
 *     Credit: 50
 *   >>> STATE: FULL_CREDIT  (LED blinking - press DISPENSE)
 *   ...
 *
 * =========================================================================
 * Author    : FSM Demo Project
 * Target    : AVR128DA48 Curiosity Nano
 * Toolchain : Atmel Studio 7 / avr-gcc
 * Clock     : 4 MHz internal OSCHF (reset default)
 * =========================================================================
 */

#include <avr/interrupt.h>   /* sei() */
#include "hal.h"
#include "vending.h"

/* =========================================================================
 * Application context instance
 *
 * Declared at file scope (static storage duration) so it is zero-initialised
 * before vending_init() is called.  Placing it here also keeps main() clean.
 * ========================================================================= */
static vend_ctx_t g_vend;

/* =========================================================================
 * main()
 * ========================================================================= */

/**
 * @brief Application entry point.
 *
 * Performs the following in order:
 *   1. hal_init()      - configures all peripherals (GPIO, UART, ADC, TCB0).
 *   2. vending_init()  - builds the FSM transition table, prints boot banner,
 *                        and fires the IDLE on_entry hook.
 *   3. sei()           - enables global interrupts (TCB0 and ADC0 ISRs).
 *   4. Superloop       - calls vending_run() indefinitely.
 *
 * The superloop is deliberately bare: all application logic is inside
 * vending_run() so main() stays readable at a glance.
 *
 * @return Never returns (embedded superloop).
 */
int main(void)
{
    /* Initialise all hardware peripherals */
    hal_init();

    /* Initialise the vending machine FSM */
    vending_init(&g_vend);

    /* Kick off the first ADC conversion so results arrive from the start */
    hal_adc_start();

    /* Enable global interrupts - TCB0 (tick) and ADC0 (result) ISRs live */
    sei();

    /*
     * Superloop
     *
     * vending_run() is non-blocking: it polls inputs, checks timers,
     * dispatches events to the FSM, and updates the LED - all without
     * any calls to delay().  The loop runs as fast as the CPU allows;
     * real-time behavior is achieved entirely through elapsed-time checks
     * against the 1 ms system tick provided by TCB0.
     */
    while (1)
    {
        vending_run(&g_vend);
    }

    /* Unreachable - suppress compiler warning */
    return 0;
}