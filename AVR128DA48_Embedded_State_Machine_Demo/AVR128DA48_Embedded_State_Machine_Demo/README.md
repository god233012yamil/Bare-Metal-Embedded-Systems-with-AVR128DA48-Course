# AVR128DA48 Embedded State Machine Demo

This Atmel Studio 7 project demonstrates a non-blocking, event-driven finite state machine on the AVR128DA48 MCU.

## Target

- MCU: AVR128DA48
- IDE: Atmel Studio 7
- Compiler: AVR-GCC
- CPU clock: 24 MHz internal oscillator
- UART: USART0, 115200 baud, 8-N-1

## Default Hardware Mapping

The project targets the AVR128DA48 Curiosity Nano pin arrangement where practical.

| Signal | Pin | Electrical behavior |
|---|---|---|
| Status LED | PF5 | Active low |
| Push button | PF6 | Active low, internal pull-up enabled |
| Motor-enable demo output | PC0 | Active high |
| Fault input | PC1 | Active low, internal pull-up enabled |
| USART0 TX | PA0 | 115200 baud |
| USART0 RX | PA1 | 115200 baud |

PC0 is a logic-level demonstration output. Do not connect a motor directly to the MCU pin. Use a suitable MOSFET, gate driver, motor driver, flyback protection, and power supply.

## State Machine

The application uses five states:

- IDLE
- STARTING
- RUNNING
- STOPPING
- FAULT

State flow:

```text
IDLE --button--> STARTING --1 second--> RUNNING
RUNNING --button--> STOPPING --500 ms--> IDLE
STARTING --button--> STOPPING
Any active state --PC1 low--> FAULT
FAULT --button with PC1 high--> IDLE
```

## LED Behavior

- IDLE: Off
- STARTING: On
- RUNNING: Toggles every 250 ms
- STOPPING: On
- FAULT: Toggles every 100 ms

## Project Structure

```text
AVR128DA48_Embedded_State_Machine.atsln
AVR128DA48_Embedded_State_Machine/
  AVR128DA48_Embedded_State_Machine.cproj
  README.md
  src/
    app.c
    app.h
    button.c
    button.h
    event_queue.c
    event_queue.h
    hardware.c
    hardware.h
    main.c
    system_time.c
    system_time.h
    uart.c
    uart.h
```

## Opening and Building

1. Open `AVR128DA48_Embedded_State_Machine.atsln` in Atmel Studio 7.
2. Confirm that the selected device is `AVR128DA48`.
3. Select Debug or Release.
4. Build the solution.
5. Connect an Atmel-ICE or use the Curiosity Nano onboard debugger.
6. Select the programmer and UPDI interface in Device Programming.
7. Program the generated ELF or HEX file.

## Operation

1. After reset, the firmware enters IDLE.
2. Press PF6 to enter STARTING. PC0 goes high.
3. After one second, the firmware enters RUNNING.
4. Press PF6 again to enter STOPPING. PC0 immediately goes low.
5. After 500 ms, the firmware returns to IDLE.
6. Pull PC1 low to force FAULT from any normal operating state.
7. Release PC1 and press PF6 to clear the fault and return to IDLE.

## Design Features

- Explicit state and event types
- Centralized entry, exit, and transition functions
- Fixed-size event queue with overflow detection
- Non-blocking timeouts using TCB0
- Wraparound-safe deadline comparisons
- Non-blocking button debounce state machine
- Safe output handling during faults and invalid-state recovery
- UART transition trace for debugging
- No dynamic memory allocation
