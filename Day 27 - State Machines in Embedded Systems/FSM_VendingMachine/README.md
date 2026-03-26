# FSM Vending Machine Demo
## AVR128DA48 Curiosity Nano — Atmel Studio 7

A complete Finite State Machine (FSM) demonstration project in embedded C,
implementing a coin-operated vending machine controller.

---

## Project Structure

```
FSM_VendingMachine/
├── FSM_VendingMachine.atsln          ← Open this in Atmel Studio 7
├── README.md
└── FSM_VendingMachine/
    ├── FSM_VendingMachine.cproj      ← GCC C Executable, AVR128DA48
    ├── fsm.h / fsm.c                 ← Generic reusable FSM engine
    ├── hal.h / hal.c                 ← Hardware Abstraction Layer
    ├── vending.h / vending.c         ← Vending machine FSM application
    └── main.c                        ← Entry point and superloop
```

### Layer responsibilities

| File | Responsibility |
|------|---------------|
| `fsm.h/.c` | Generic table-driven FSM engine. Zero application knowledge. Reusable in any project. |
| `hal.h/.c` | All register-level AVR128DA48 code. GPIO, UART, ADC0, TCB0 tick. |
| `vending.h/.c` | FSM states, events, transition table, action functions, LED patterns. |
| `main.c` | Calls `hal_init()`, `vending_init()`, `sei()`, then loops on `vending_run()`. |

---

## State Machine

```
              [COIN]               [COIN]
  IDLE ──────────────► HAS_CREDIT ──────────► FULL_CREDIT
   ▲                       │                      │   │
   │         CANCEL/timeout│       CANCEL/timeout │   │ [DISPENSE]
   │◄──────────────────────┴──────────────────────┘   │
   │                                                   ▼
   │  [RESET, auto after 2s]                      DISPENSING
   │◄──────────────────────────────────────────────────┘
   │
   │  [FAULT, ADC > 2.5V]    ┌─────────┐
   │◄────────────────────────│  FAULT  │
        [SERVICE, SW0+DISP]  └─────────┘
```

| State | LED Behaviour |
|-------|--------------|
| IDLE | OFF |
| HAS_CREDIT | ON (steady) |
| FULL_CREDIT | Slow blink (500 ms) |
| DISPENSING | Fast blink (100 ms) |
| FAULT | SOS Morse pattern |

---

## Hardware Connections

| Signal | MCU Pin | Notes |
|--------|---------|-------|
| LED0 | PC6 | Onboard LED, active LOW |
| SW0 (Cancel) | PC7 | Onboard button, active LOW, pull-up |
| COIN button | PA2 | External button to GND, pull-up enabled |
| DISPENSE button | PA3 | External button to GND, pull-up enabled |
| ADC sensor | PD3 / AIN3 | Potentiometer between VDD and GND |
| UART TX | PA0 | Connect to USB-UART bridge, 9600 8N1 |

> **External buttons:** wire a momentary push-button between the pin and GND.
> The internal pull-up is enabled in software; no external resistor is needed.

---

## How to Build

1. Open `FSM_VendingMachine.atsln` in **Atmel Studio 7** or **Microchip Studio**.
2. Verify that **AVR-Dx Device Pack 2.4.286** is installed
   (*Tools → Device Pack Manager*).
3. Select the **Debug** configuration.
4. Press **F7** (Build Solution). Zero errors and warnings expected.
5. Connect the Curiosity Nano via USB.
6. Press **Ctrl+Alt+F5** (Start Without Debugging) to flash and run.

---

## How to Operate

Open a serial terminal (e.g. PuTTY, Tera Term) at **9600 baud, 8N1** on the
COM port of the Curiosity Nano's on-board debugger.

| Action | Button | FSM Event |
|--------|--------|-----------|
| Insert first coin | PA2 | COIN → IDLE transitions to HAS_CREDIT |
| Insert second coin | PA2 | COIN → HAS_CREDIT transitions to FULL_CREDIT |
| Select product | PA3 | DISPENSE → FULL_CREDIT transitions to DISPENSING |
| Wait 2 seconds | — | Auto RESET → DISPENSING returns to IDLE |
| Cancel / refund | PC7 (SW0) | CANCEL → any credit state returns to IDLE |
| Trigger fault | Raise PD3 > 2.5V | FAULT → any state transitions to FAULT |
| Service reset | SW0 + PA3 simultaneously | SERVICE → FAULT returns to IDLE |
| Idle timeout | No input for 10 s | Auto CANCEL → credit states return to IDLE |

---

## UART Output Example

```
========================================
  FSM Vending Machine Demo
  AVR128DA48 Curiosity Nano
----------------------------------------
  Buttons:
    PA2 (COIN)     - Insert 25c coin
    PA3 (DISPENSE) - Select product
    PC7 (SW0)      - Cancel / refund
    SW0+DISPENSE   - Service reset
  ADC on PD3: raise voltage > 2.5V
             to simulate fault.
========================================

>>> STATE: IDLE  (LED off - waiting for coin)
[FSM] EVENT: COIN  |  IDLE -> HAS_CREDIT
[ACT] Coin inserted (+25 c)
  Credit: 25
>>> STATE: HAS_CREDIT  (LED on - insert one more coin)
[FSM] EVENT: COIN  |  HAS_CREDIT -> FULL_CREDIT
[ACT] Coin inserted (+25 c)
  Credit: 50
>>> STATE: FULL_CREDIT  (LED blinking - press DISPENSE)
[FSM] EVENT: DISPENSE  |  FULL_CREDIT -> DISPENSING
[ACT] DISPENSING product!
  Price paid: 50
  Change due: 0
>>> STATE: DISPENSING  (LED fast-blink - vending...)
[TMR] Dispense timeout - resetting.
[FSM] EVENT: RESET  |  DISPENSING -> IDLE
[ACT] Dispense complete.
>>> STATE: IDLE  (LED off - waiting for coin)
```

---

## FSM Engine API (fsm.h)

The generic engine is self-contained and reusable:

```c
/* Define your states and events as enums, then build a table: */
static const fsm_transition_t my_table[] = {
    { STATE_A, EVENT_X, action_fn, STATE_B },
    { STATE_B, EVENT_Y, NULL,      STATE_A },
    { FSM_ANY, EVENT_Z, reset_fn,  STATE_A },  /* wildcard */
};

fsm_t fsm;
fsm_init(&fsm, STATE_A, my_table, ARRAY_LEN(my_table), hooks, N_STATES, ctx);
fsm_dispatch(&fsm, EVENT_X);   /* fires action_fn, moves to STATE_B */
```

---

## Clock and Timing

| Parameter | Value |
|-----------|-------|
| System clock | 4 MHz (OSCHF default, no configuration needed) |
| TCB0 period | 1 ms (system tick for timeouts and debounce) |
| ADC clock | 250 kHz (CLK_PER / 16) |
| UART baud | 9600 |
| Button debounce | 20 ms |
| Idle timeout | 10 000 ms |
| Dispense time | 2 000 ms |
| Fault threshold | ADC ≥ 3103 (≈ 2.5 V with VDD reference) |
