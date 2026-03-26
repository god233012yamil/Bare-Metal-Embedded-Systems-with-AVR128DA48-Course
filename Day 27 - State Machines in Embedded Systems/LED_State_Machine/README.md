# LED Controller – State Machine
## AVR128DA48 Curiosity Nano

### Overview
Implements a three-state LED controller on the AVR128DA48 Curiosity Nano board.

| State   | LED Behaviour         |
|---------|-----------------------|
| IDLE    | OFF                   |
| RUNNING | Blink  (~500 ms period) |
| ERROR   | Fast Blink (~125 ms period) |

### Transitions

| From    | Trigger                        | To      |
|---------|-------------------------------|---------|
| IDLE    | Button press (SW0 / PC7)      | RUNNING |
| RUNNING | ADC >= threshold (~2.5 V)     | ERROR   |
| ERROR   | Button press (SW0 / PC7)      | IDLE    |

### Hardware Connections (Curiosity Nano)

| Signal      | MCU Pin | Notes                                      |
|-------------|---------|---------------------------------------------|
| LED0        | PC6     | Onboard LED, active LOW                    |
| SW0 (Button)| PC7     | Onboard button, active LOW, internal pull-up |
| ADC input   | PD3     | AIN3 – connect a pot between VDD and GND   |

### ADC Threshold
- Reference: VDD (~3.3 V)
- Resolution: 12-bit (0–4095)
- Threshold: 3103  (~2.5 V)
- Adjust `ADC_THRESHOLD` in `main.c` to change the trigger level.

### Clock
- 4 MHz internal oscillator (default after reset, no configuration needed)

### How to Build
1. Open `LED_StateMachine.atsln` in Atmel Studio 7 / Microchip Studio.
2. Ensure AVR-Dx Device Pack **2.4.286** (or compatible) is installed.
3. Select **Debug** or **Release** configuration.
4. Build → **Build Solution** (F7).
5. Connect the Curiosity Nano board and program via **Debug → Start Without Debugging** (Ctrl+Alt+F5).

### Operating the Demo
1. Power-on → IDLE state: LED is OFF.
2. Press SW0 → transitions to RUNNING: LED blinks at ~1 Hz.
3. Raise the voltage on PD3 above ~2.5 V → transitions to ERROR: LED fast-blinks at ~4 Hz.
4. Press SW0 again → transitions back to IDLE: LED turns OFF.
