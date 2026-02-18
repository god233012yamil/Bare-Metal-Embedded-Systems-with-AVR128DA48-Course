# Bare-Metal Embedded Systems with AVR128DA48

**From GPIO to Real-World Peripheral Integration in 30 Days**

A structured 30-day course for learning register-level embedded programming on the AVR128DA48 microcontroller. No code generators, no abstraction layers, no libraries &mdash; just you, the datasheet, and the hardware.

<p align="center">
  <img src="AVR128DA48 Curiosity Nano.png" alt="AVR128DA48 Curiosity Nano Board" width="600">
</p>

---

## Why This Course?

Most embedded tutorials rely on HALs, code generators (MCC/START), or Arduino abstractions that hide what the hardware is actually doing. This course takes the opposite approach: every peripheral is configured by writing directly to registers, giving you a deep understanding of the silicon.

By the end, you will be able to:

- Read and apply any section of the AVR128DA48 datasheet
- Configure clocks, GPIOs, timers, USART, SPI, I2C, ADC, DAC, and more from scratch
- Design clean, reusable bare-metal drivers
- Build integrated embedded systems with multiple peripherals working together

## Target Audience

- Beginner to intermediate embedded engineers
- Engineers transitioning from Arduino or HAL-based development
- Anyone who wants strong register-level AVR experience

## Hardware & Tools

| Component | Details |
|-----------|---------|
| **MCU** | [AVR128DA48](https://www.microchip.com/en-us/product/AVR128DA48) |
| **Board** | [AVR128DA48 Curiosity Nano (DM164151)](https://www.microchip.com/en-us/development-tool/DM164151) |
| **IDE** | [Microchip Studio (formerly Atmel Studio 7)](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads) |
| **Language** | C (bare metal &mdash; no ASF, no MCC code generation) |
| **Debugger** | On-board UPDI debugger (built into the Curiosity Nano) |

<p align="center">
  <img src="AVR128DA48 Curiosity Nano Pinout.png" alt="AVR128DA48 Curiosity Nano Pinout" width="700">
</p>

## Course Structure

The progression is intentional: **fundamentals first, peripherals next, then integration and real-world patterns**.

### Week 1 &mdash; Core MCU Fundamentals (Days 1&ndash;5)

| Day | Topic | Lab Projects |
|-----|-------|-------------|
| 1 | [Course Introduction and Board Bring-Up](Day%201%20-%20Course%20Introduction%20and%20Board%20Bring-Up/) | `day01_bringup` &mdash; Compile and flash an empty project |
| 2 | [AVR128DA48 Architecture Deep Dive](Day%202%20-%20AVR128DA48%20Architecture%20Deep%20Dive/) | Inspect memory map in debugger (presentation only) |
| 3 | [Clock System (CLKCTRL)](Day%203%20-%20Clock%20System%20(CLKCTRL)/) | `day03_proving_clock_frequency` &mdash; Configure system clock and verify with GPIO toggle |
| 4 | [GPIO at Register Level](Day%204%20-%20GPIO%20at%20Register%20Level/) | `day04_led_control_without_delays` &mdash; LED blink and button input without delay loops |
| 5 | [Reset System and Fuse Basics](Day%205%20-%20Reset%20System%20and%20Fuse%20Basics/) | `day05_reading_clearing_reset_flags`, `day05_read_all_fuses`, `day05_read_all_fuses_2`, `day05_runtime_fuse_write_demo`, `day05_software_reset_demonstration`, `day05_watchdog_reset_demonstration` |

### Week 2 &mdash; Timers and Timekeeping (Days 6&ndash;10)

| Day | Topic | Lab Projects |
|-----|-------|-------------|
| 6 | [Introduction to Timers on AVR DA](Day%206%20-%20Introduction%20to%20Timers%20on%20AVR%20DA/) | `day06_tca0_periodic_ovf_int_mode`, `day06_tcb0_periodic_ovf_int_mode`, `day06_tcb0_periodic_cap_int_mode` |
| 7 | [TCA Single Mode (PWM Basics)](Day%207%20-%20TCA%20Single%20Mode%20(PWM%20Basics)/) | `tcao_pwm_on_wo0`, `tcao_pwm_on_wo0_2` &mdash; Single-slope PWM with runtime frequency/duty control |
| 8 | [TCA Split Mode](Day%208%20-%20TCA%20Split%20Mode/) | `tca0_split_mode_pwm_4ch` &mdash; Four independent PWM channels |
| 9 | [TCB Timers (Periodic Interrupts and One-Shot Timing)](Day%209%20-%20TCB%20Timers%20(Periodic%20Interrupts%20and%20One-Shot%20Timing)/) | `tcb0_periodic_1ms_interrupt`, `tcb1_one_shot_timer` |
| 10 | [Real-Time Counter (RTC) Basics](Day%2010%20-%20Real-Time%20Counter%20(RTC)%20Basics/) | `rtc_1hz_periodic_interrupt`, `rtc_1hz_periodic_int_sleep`, `rtc_sleep_demo` |

### Week 3 &mdash; Communication Peripherals (Days 11&ndash;16)

| Day | Topic | Lab Projects |
|-----|-------|-------------|
| 11 | [USART Fundamentals](Day%2011%20-%20USART%20Fundamentals/) | `usart0_polling_demo` &mdash; Polling-based UART TX/RX to PC terminal |
| 12 | [USART Advanced Usage](Day%2012%20-%20USART%20Advanced%20Usage/) | `usart0_interrupt_demo_1`, `usart0_interrupt_demo_2` &mdash; Interrupt-driven TX/RX with ring buffers and command parser |
| 13 | SPI Master Mode | *Coming soon* |
| 14 | SPI Architecture Patterns | *Coming soon* |
| 15 | I2C (TWI) Fundamentals | *Coming soon* |
| 16 | I2C Advanced Driver Design | *Coming soon* |

### Week 4 &mdash; Analog and System Peripherals (Days 17&ndash;22)

| Day | Topic | Lab Projects |
|-----|-------|-------------|
| 17 | ADC Fundamentals | *Coming soon* |
| 18 | ADC Advanced Topics | *Coming soon* |
| 19 | DAC Peripheral | *Coming soon* |
| 20 | Analog Comparator (AC) | *Coming soon* |
| 21 | Event System | *Coming soon* |
| 22 | Sleep Modes and Power Management | *Coming soon* |

### Week 5 &mdash; Architecture, Reliability, and Integration (Days 23&ndash;30)

| Day | Topic | Lab Projects |
|-----|-------|-------------|
| 23 | Interrupt Architecture | *Coming soon* |
| 24 | Watchdog Timer (WDT) | *Coming soon* |
| 25 | EEPROM and Non-Volatile Data | *Coming soon* |
| 26 | Bare-Metal Driver Architecture | *Coming soon* |
| 27 | Cooperative Task Scheduling | *Coming soon* |
| 28 | Debugging Techniques | *Coming soon* |
| 29 | System Integration Day | *Coming soon* |
| 30 | Final Capstone Project | *Coming soon* |

## Repository Structure

```
.
├── Day 1 - Course Introduction and Board Bring-Up/
│   ├── Day 1 - Course Introduction and Board Bring-Up.pptx
│   └── day01_bringup/                  # Microchip Studio project
├── Day 2 - AVR128DA48 Architecture Deep Dive/
│   └── Day 2 - AVR128DA48 Architecture Deep Dive.pptx
├── Day 3 - Clock System (CLKCTRL)/
│   ├── Day 3 - Clock System (CLKCTRL).pptx
│   └── day03_proving_clock_frequency/
├── ...
├── Day 12 - USART Advanced Usage/
│   ├── Day 12 - USART Advanced Usage.pptx
│   ├── usart0_interrupt_demo_1/
│   └── usart0_interrupt_demo_2/
├── AVR128DA48 Curiosity Nano.png
├── AVR128DA48 Curiosity Nano Pinout.png
├── AVR® CPU Architecture.png
└── README.md
```

Each day folder contains:
- A **PowerPoint presentation** (`.pptx`) covering the theory and register details
- One or more **Microchip Studio C projects** (GCC C Executable) demonstrating the concepts with fully commented source code

## Getting Started

### 1. Get the Hardware

Purchase the [AVR128DA48 Curiosity Nano](https://www.microchip.com/en-us/development-tool/DM164151) evaluation board. It includes an on-board debugger/programmer &mdash; just plug in a USB cable.

### 2. Install the IDE

Download and install [Microchip Studio](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads) (formerly Atmel Studio 7). Make sure to install the **AVR-Dx Device Family Pack** when prompted.

### 3. Clone This Repository

```bash
git clone https://github.com/god233012yamil/Bare-Metal-Embedded-Systems-with-AVR128DA48-Course.git
```

### 4. Open a Project

1. Launch Microchip Studio
2. Go to **File > Open > Project/Solution**
3. Navigate to any day's project folder and open the `.atsln` or `.cproj` file
4. Connect the Curiosity Nano board via USB
5. Build (**F7**) and flash (**Ctrl+Alt+F5**)

### 5. Follow Along

Open the corresponding `.pptx` presentation for each day to understand the theory before examining the code. The presentations cover register maps, bit fields, and configuration rationale in detail.

## Key Topics Covered

```
MCU Architecture     │  Clock System (CLKCTRL)   │  GPIO Registers (PORT)
Reset Sources        │  Fuse Configuration        │  TCA Timer (Normal/PWM/Split)
TCB Timer            │  RTC (32 kHz domain)       │  USART (Polling & Interrupts)
Ring Buffers         │  SPI Master                │  I2C / TWI
ADC (with Window)    │  DAC Output                │  Analog Comparator
Event System         │  Sleep Modes               │  Watchdog Timer
EEPROM Access        │  Interrupt Architecture    │  Driver Design Patterns
Cooperative Scheduler│  Debugging Techniques      │  System Integration
```

## Resources

- [AVR128DA48 Product Page](https://www.microchip.com/en-us/product/AVR128DA48)
- [AVR128DA48 Datasheet (PDF)](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR128DA28-32-48-64-Data-Sheet-DS40002183.pdf)
- [AVR128DA48 Curiosity Nano Kit](https://www.microchip.com/en-us/development-tool/DM164151)
- [Microchip PIC & AVR Examples (GitHub)](https://github.com/microchip-pic-avr-examples)

## Current Progress

> **Days 1&ndash;12 are complete** with presentations and working lab projects.
> Days 13&ndash;30 are planned and will be added as they are developed.

Contributions, suggestions, and issue reports are welcome.

## License

This project is provided for educational purposes. See [LICENSE](LICENSE) for details.
