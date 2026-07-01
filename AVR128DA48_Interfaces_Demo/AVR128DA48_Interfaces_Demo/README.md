# AVR128DA48 Interfaces Demo

This Atmel Studio 7 project accompanies the article `Interfaces in C: Reducing Coupling in Embedded Firmware Design`.

## Target

- MCU: AVR128DA48
- IDE: Atmel Studio 7 / Microchip Studio
- Toolchain: AVR GCC
- Clock: Internal 4 MHz oscillator

## Demo behavior

- PA0 toggles every 500 ms through a generic LED interface.
- USART0 transmits a small packet every 500 ms through a generic communication interface.
- Application and protocol code depend on interfaces, not on concrete GPIO or USART drivers.

## Pins used

- PA0: Status LED output
- PC0: USART0 TXD
- PC1: USART0 RXD

## Main design idea

The application receives these interfaces:

```c
typedef struct {
    void *context;
    void (*set)(void *context, bool state);
    void (*toggle)(void *context);
} led_interface_t;

typedef struct {
    void *context;
    int (*write)(void *context, const uint8_t *data, size_t length);
    int (*read)(void *context, uint8_t *data, size_t length);
} comm_interface_t;
```

The concrete implementation may be GPIO, USART, or a mock object used for tests. This keeps the high-level application code loosely coupled to the hardware.
