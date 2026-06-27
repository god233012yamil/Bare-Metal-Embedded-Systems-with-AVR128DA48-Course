# AVR128DA48 Callback and Event-Driven Demo

This Atmel Studio 7 project accompanies the article "Callbacks in C: Building Flexible and Event-Driven Embedded Systems."

## Target

- MCU: AVR128DA48
- IDE: Atmel Studio 7
- Compiler: AVR-GCC supplied by Atmel Studio
- Clock: Reset-default 4 MHz internal oscillator
- Board mapping: AVR128DA48 Curiosity Nano
- LED: PC6, active low
- Button: PC7, active low with internal pull-up

## Behavior

1. PC7 generates a falling-edge interrupt when the user button is pressed.
2. The button driver invokes a registered callback from interrupt context.
3. The callback posts `APP_EVENT_BUTTON_PRESSED` to a fixed-size event queue.
4. The main loop processes the event and toggles the PC6 LED.
5. A one-shot software timer starts for 1000 ms.
6. The timer callback posts `APP_EVENT_TIMEOUT` from the 1 ms TCB0 interrupt.
7. The main loop processes the timeout event and turns the LED off.
8. When no event is pending, the CPU enters idle sleep.

## Architecture

- `board.c`: Board-specific GPIO operations.
- `button.c`: Button callback registration and interrupt-side debounce.
- `system_tick.c`: TCB0-based 1 ms system tick.
- `software_timer.c`: One-shot timer with callback and context pointer.
- `app_event.c`: Interrupt-safe static ring-buffer event queue.
- `interrupts.c`: Hardware interrupt vectors.
- `main.c`: Callback registration, deferred event handling, and sleep loop.

## Opening the project

1. Install the latest AVR-Dx device pack available for Atmel Studio 7.
2. Extract the ZIP archive.
3. Open `AVR128DA48_Callbacks_Event_Driven.atsln`.
4. Select the AVR128DA48 Curiosity Nano or an Atmel-ICE programmer.
5. Build the solution.
6. Program and debug the MCU.

## Custom hardware

For a custom AVR128DA48 board, modify these definitions in `src/board.c`:

```c
#define BOARD_LED_PIN PIN6_bm
#define BOARD_BUTTON_PIN PIN7_bm
```

Also update the selected port registers if the LED or button is not connected to PORTC.

## Notes

- Callback code reached from an ISR must remain short and non-blocking.
- The application callbacks only enqueue events. Application processing occurs in the main loop.
- Queue storage is static; no dynamic memory allocation is used.
- The event queue tracks dropped events when it becomes full.
- The optional Makefile supports command-line AVR-GCC builds when the installed compiler recognizes `avr128da48`.
