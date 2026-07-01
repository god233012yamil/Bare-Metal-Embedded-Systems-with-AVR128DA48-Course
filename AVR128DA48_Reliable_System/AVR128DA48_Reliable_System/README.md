# AVR128DA48 Reliable Embedded System Demo

This Atmel Studio 7 project accompanies the article "Designing Reliable Embedded Systems That Never Crash".

## Target

- MCU: AVR128DA48
- Toolchain: AVR/GNU C Compiler in Atmel Studio 7 or Microchip Studio
- Clock: 24 MHz internal oscillator
- UART: USART0, 115200 baud, 8N1

## Demonstrated Reliability Patterns

- Cooperative non-blocking task loop
- Central system state machine
- Watchdog servicing based on task health flags
- Fault logging to EEPROM
- Reset reason capture
- Failed boot counter and safe mode entry
- Input validation
- Timeout-protected UART driver
- Safe output state handling
- Button-triggered controlled fault injection
- Periodic diagnostics over UART

## Demo Pinout

- Heartbeat LED: PB3
- Fault injection button: PC7, active low with internal pull-up
- USART0 TX: PA0
- USART0 RX: PA1

## How to Use

1. Open `AVR128DA48_Reliable_System.atsln` in Atmel Studio 7.
2. Build the `AVR128DA48_Reliable_System` project.
3. Program the AVR128DA48 using Atmel-ICE or a compatible programmer.
4. Open a serial terminal at 115200 baud.
5. Press the PC7 button to force a controlled fault and observe recovery.

## Notes

The sensor task uses a simulated temperature value so the project can run without external hardware. Replace `sensor_read()` with a real ADC, I2C, SPI, or UART sensor driver when adapting this project to real hardware.
