# AVR128DA48 RTC Sleep Wake-up Demonstration

## Project Overview

This project demonstrates proper use of the AVR128DA48's Sleep Controller (SLPCTRL) in combination with the Real-Time Counter (RTC) to achieve low-power periodic wake-up operation. The MCU sleeps in Power-Down mode and wakes every second to toggle the on-board LED.

## Hardware Requirements

- **Board**: AVR128DA48 Curiosity Nano (DM164151)
- **LED**: On-board LED connected to PC6
- **Debugger**: On-board nEDBG debugger (built-in)

## Software Requirements

- **IDE**: Atmel Studio 7 or Microchip Studio
- **Toolchain**: AVR GCC
- **Device Pack**: AVR-Dx Device Pack v2.4.286 or later
- **Project Type**: GCC C Executable Project

## Features Demonstrated

1. **Sleep Mode Configuration**: Power-Down mode for minimum power consumption
2. **RTC Configuration**: 32.768 kHz internal oscillator with 1024 Hz tick rate
3. **Interrupt-Driven Wake-up**: RTC overflow interrupt wakes MCU every 1 second
4. **Low-Power Operation**: MCU sleeps between wake-ups (~5-10 µA typical)
5. **Visual Feedback**: LED toggles on each wake-up

## Clock Configuration

- **Main Clock**: 4 MHz (24 MHz OSCHF / 6 prescaler) - Default
- **RTC Clock**: 32.768 kHz internal oscillator (OSC32K)
- **RTC Prescaler**: DIV32 (1024 Hz tick rate)
- **RTC Period**: 1024 ticks = 1 second

## Sleep Modes Available on AVR128DA48

| Mode       | CPU | Main Clock | Peripherals | Wake Sources | Current (Typ) |
|------------|-----|------------|-------------|--------------|---------------|
| Active     | ON  | ON         | ON          | N/A          | ~4 mA         |
| Idle       | OFF | ON         | ON          | Any Interrupt| ~2 mA         |
| Standby    | OFF | OFF        | Selected    | Wake Pins    | ~50 µA        |
| Power-Down | OFF | OFF        | RTC/WDT     | Wake Pins    | ~5-10 µA      |

*This project uses **Power-Down** mode for maximum power savings.*

## Code Structure

### Main Components

1. **clock_init()**: System clock initialization (default 4 MHz)
2. **gpio_init()**: LED pin configuration
3. **rtc_init()**: RTC configuration with 1-second period
4. **sleep_init()**: Sleep controller setup for Power-Down mode
5. **ISR(RTC_CNT_vect)**: RTC interrupt handler (toggles LED)
6. **main()**: Initialization and sleep loop

### Program Flow

```
[Power On]
    ↓
[Initialize Clock, GPIO, RTC, Sleep]
    ↓
[Enable Interrupts]
    ↓
[Startup Blink (3x)]
    ↓
┌─────────────────┐
│ Enter Sleep     │ ← Power-Down mode (~5-10 µA)
└────────┬────────┘
         ↓ (1 second)
┌─────────────────┐
│ RTC Interrupt   │ ← Wake-up
│ Toggle LED      │
└────────┬────────┘
         ↓
[Return to Sleep]
         ↑
         └──────── Loop Forever
```

## Building the Project

### Using Atmel Studio 7 / Microchip Studio

1. **Create New Project**:
   - File → New → Project
   - Select "GCC C Executable Project"
   - Name: `AVR_RTC_Sleep_Demo`
   - Click OK

2. **Select Device**:
   - Choose "AVR128DA48"
   - Click OK

3. **Add Source File**:
   - Replace the default `main.c` with the provided code
   - Or copy the contents into the existing `main.c`

4. **Build Configuration**:
   - Build → Configuration Manager
   - Select "Debug" or "Release"
   - Build the project (F7)

5. **Program Device**:
   - Connect Curiosity Nano board via USB
   - Tools → Device Programming
   - Tool: nEDBG, Device: AVR128DA48, Interface: UPDI
   - Click "Apply"
   - Select "Memories" → Program (Ctrl+Shift+P)

### Compiler Settings (Optional Optimization)

For minimum power consumption, consider these optimizations:

- **Optimization Level**: -Os (Optimize for size)
- **Link-Time Optimization**: Enable
- **Fuse Settings**: Verify BOD (Brown-Out Detection) settings

## Expected Behavior

1. **Power-On**: LED blinks 3 times quickly (startup indication)
2. **Normal Operation**: LED toggles every 1 second
3. **Between Toggles**: MCU is in Power-Down sleep mode
4. **Current Draw**: Should measure ~5-10 µA during sleep

## Power Consumption Measurement

To measure actual power consumption:

1. **Remove J101 jumper** on Curiosity Nano board
2. **Connect ammeter** between J101 pins
3. **Observe**:
   - Sleep periods: ~5-10 µA (depending on peripherals)
   - Active periods: ~4 mA (brief, during LED toggle)
   - Average: Will depend on wake frequency

## Customization Options

### Change Wake-up Period

Modify `RTC_PERIOD` constant:

```c
// For 2-second wake-up:
#define RTC_PERIOD 2048  // 2048 ticks = 2 seconds

// For 500 ms wake-up:
#define RTC_PERIOD 512   // 512 ticks = 0.5 seconds
```

### Change Sleep Mode

Modify sleep mode in `sleep_init()`:

```c
// For Standby mode (higher power, faster wake):
SLPCTRL.CTRLA = SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm;

// For Idle mode (highest power, peripherals running):
SLPCTRL.CTRLA = SLPCTRL_SMODE_IDLE_gc | SLPCTRL_SEN_bm;
```

### Add Work After Wake-up

Add processing in main loop after `sleep_cpu()`:

```c
while (1)
{
    sleep_cpu();
    
    // MCU is awake here after interrupt
    // Do additional work:
    // - Read sensors
    // - Process data
    // - Update display
    // Then sleep again on next loop iteration
}
```

## Troubleshooting

### LED Not Blinking

1. **Check Programming**: Verify device is programmed correctly
2. **Check RTC Clock**: Ensure OSC32K is enabled and stable
3. **Check Interrupts**: Verify `sei()` is called
4. **Check UPDI Fuse**: Ensure UPDI is not disabled

### Unexpected Current Draw

1. **Check Pin States**: Floating pins draw current
2. **Disable Unused Peripherals**: Turn off ADC, DAC, etc.
3. **Check Pull-ups**: Internal pull-ups consume current
4. **Verify Sleep Mode**: Ensure Power-Down is selected

### RTC Not Running

1. **OSC32K Startup**: May need longer delay for stabilization
2. **Check RUNSTDBY**: Must be enabled for sleep operation
3. **Verify Clock Source**: Should be INT32K_gc

## Key Learning Points

1. **RTC Configuration**: Proper setup of internal 32 kHz oscillator
2. **Sleep Controller**: How to configure different sleep modes
3. **Interrupt-Based Wake**: Using interrupts to exit sleep
4. **Low-Power Design**: Balancing power consumption with functionality
5. **Register Synchronization**: Waiting for RTC registers to sync

## Further Enhancements

Consider adding:

1. **External Wake Sources**: PIN change interrupts
2. **Multiple RTC Compares**: More complex timing
3. **USART Communication**: Status reporting after wake
4. **Sensor Reading**: Periodic data acquisition
5. **Battery Monitoring**: ADC-based voltage measurement

## References

- AVR128DA48 Datasheet (DS40002183)
- AVR128DA Family User Guide
- Microchip Application Note AN2543: Sleep Modes
- AVR-Dx Device Pack Documentation

## License

This code is provided as an educational example for embedded systems development.

## Author

Created for AVR128DA48 embedded development demonstration (2026)
