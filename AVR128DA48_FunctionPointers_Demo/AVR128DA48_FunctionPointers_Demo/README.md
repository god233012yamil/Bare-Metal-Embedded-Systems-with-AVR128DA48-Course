# AVR128DA48 Function Pointers in Embedded C Demo

This Atmel Studio 7 / Microchip Studio project accompanies the article:

Function Pointers in Embedded C: Powerful Design Techniques for Flexible Firmware

## Target

- MCU: AVR128DA48
- Toolchain: Atmel Studio 7 or Microchip Studio with AVR GCC
- Clock: Internal oscillator, 4 MHz default assumption
- UART: USART0, 9600 baud

## Demonstrated techniques

- Callback registration
- Function pointers with context pointers
- Function pointer based LED interface
- Debounced button event callback
- Software timer callback
- UART receive callback
- Command dispatch table
- Function pointer based state machine

## Default pin usage

- PA6: Status LED
- PA7: Heartbeat LED
- PA2: Active-low button with internal pull-up
- USART0 default pins: PA0 TXD and PA1 RXD

## UART commands

Send commands terminated with CR or LF:

- ENABLE
- DISABLE
- FAULT
- CLEAR
- STATUS

## Notes

The project is intentionally written in portable embedded C style. Hardware-specific code is isolated in board.c and uart_driver.c. The application logic demonstrates how function pointers can decouple low-level drivers from high-level firmware behavior.
