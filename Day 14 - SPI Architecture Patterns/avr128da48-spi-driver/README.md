# AVR128DA48 SPI Driver

A production-ready, fully-documented SPI driver for the **AVR128DA48** microcontroller (AVR-Dx family) with both **blocking** and **non-blocking (interrupt-driven)** transfer modes. Designed for reusability, this driver serves as a hardware abstraction layer that higher-level sensor and flash drivers can build upon without modification.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-AVR--Dx-blue.svg)](https://www.microchip.com/en-us/products/microcontrollers-and-microprocessors/8-bit-mcus/avr-mcus/avr-dx)
[![Toolchain](https://img.shields.io/badge/toolchain-Microchip%20Studio%207-green.svg)](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

---

## Table of Contents

- [Features](#features)
- [Hardware Target](#hardware-target)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
  - [Configuration](#configuration)
- [Usage](#usage)
  - [Blocking Mode](#blocking-mode)
  - [Non-Blocking Mode](#non-blocking-mode)
  - [Flash Driver Example](#flash-driver-example)
- [API Reference](#api-reference)
- [Architecture](#architecture)
- [Flowcharts](#flowcharts)
- [Examples](#examples)
- [Performance](#performance)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Features

### Core Capabilities

✅ **Dual Transfer Modes**
- **Blocking with timeout protection** — simple polling loop with cycle-counter timeout guard
- **Non-blocking (interrupt-driven)** — CPU remains free for other work while SPI bus operates autonomously

✅ **Timeout Protection**
- Software cycle counter prevents bus stalls from hanging the CPU
- Diagnostic counters track timeout events without halting execution

✅ **Ring Buffer Architecture**
- Power-of-2 ring buffers with bitmask wrap-around (no expensive modulo division)
- Separate TX and RX buffers for clean ISR logic

✅ **CS Pin Flexibility**
- Chip-select management is caller-controlled
- One driver serves multiple devices on different ports and pins

✅ **Zero External Dependencies**
- No RTOS, no hardware timers consumed
- Relies only on `<avr/io.h>`, `<avr/interrupt.h>`, and standard C library

✅ **Layered Design**
- Clean separation: `spi_driver` (hardware abstraction) ← `flash_driver` (device commands) ← application
- Device-specific drivers (flash, sensor, DAC) layer on top without modifying SPI code

### Design Philosophy

- **Reusable** — one SPI driver serves all devices (flash, sensors, DACs)
- **Testable** — blocking and non-blocking modes demonstrated on real hardware
- **Documented** — every function has a docstring; flowcharts detail internal logic
- **Safe** — timeout guards, argument validation, race-condition-free ISR handoff

---

## Hardware Target

| Component | Specification |
|-----------|---------------|
| **MCU** | AVR128DA48 @ 4 MHz (default CLK_PER, no PLL) |
| **Board** | AVR128DA48 Curiosity Nano (DM164151) |
| **Peripheral** | SPI0 (PORTC — PC0=SCK, PC1=MISO, PC2=MOSI) |
| **Chip-Select** | User-defined GPIO (example: PA4, PA5) |
| **Toolchain** | Atmel/Microchip Studio 7 — GCC C Executable Project |
| **Device Pack** | AVR-Dx 2.4.286 |

### Pin Mapping (Default PORTMUX)

```
AVR128DA48           SPI Device
──────────────────   ───────────────
PC0 (SPI0 SCK)   ──► SCK
PC1 (SPI0 MISO)  ◄── MISO
PC2 (SPI0 MOSI)  ──► MOSI
PA4 (GPIO)       ──► CS  (example)
GND              ──► GND
3V3              ──► VCC
```

---

## Project Structure

```
avr_spi_project/
│
├── include/
│   ├── spi_driver.h       ← Public API for SPI hardware abstraction
│   └── flash_driver.h     ← Device-specific flash driver (W25Q32JV)
│
├── src/
│   ├── spi_driver.c       ← SPI driver implementation + ISR
│   ├── flash_driver.c     ← Flash command sequences (Read, Erase, Write)
│   └── main.c             ← Three live demonstrations
│
├── FLOWCHART.md           ← Mermaid diagrams (13 flowcharts + 1 sequence diagram)
├── README.md              ← This file
└── LICENSE                ← MIT License
```

---

## Getting Started

### Prerequisites

1. **Hardware:**
   - AVR128DA48 Curiosity Nano board
   - SPI device (e.g., W25Q32JV flash, MCP4921 DAC, or BME280 sensor)
   - Jumper wires

2. **Software:**
   - [Microchip Studio 7](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio) (Windows)
   - OR [avr-gcc](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers) + `avrdude` (Linux/macOS)
   - AVR-Dx Device Family Pack 2.4.286 or later

### Installation

#### Option 1: Microchip Studio 7

1. **Create a new GCC C Executable Project:**
   ```
   File → New → Project → GCC C Executable Project
   Name: avr_spi_demo
   Device: AVR128DA48
   ```

2. **Add source files:**
   - Right-click project → **Add → Existing Item**
   - Add all `.c` files from `src/`

3. **Configure include path:**
   - Right-click project → **Properties**
   - Navigate to **Toolchain → AVR/GNU C Compiler → Directories**
   - Add: `../include`

4. **Build:**
   - Press `F7` or **Build → Build Solution**

5. **Program:**
   - Connect the Curiosity Nano via USB
   - Press `F5` or **Debug → Start Debugging and Break**

#### Option 2: Command-Line (Linux/macOS)

```bash
# Clone the repository
git clone https://github.com/yourusername/avr128da48-spi-driver.git
cd avr128da48-spi-driver

# Compile
avr-gcc -mmcu=avr128da48 -DF_CPU=4000000UL -Os \
    -I./include \
    -o build/main.elf \
    src/main.c src/spi_driver.c src/flash_driver.c

# Convert to hex
avr-objcopy -O ihex -R .eeprom build/main.elf build/main.hex

# Program via UPDI (adjust -P port as needed)
avrdude -c serialupdi -p avr128da48 -P /dev/ttyUSB0 -U flash:w:build/main.hex:i
```

### Configuration

Adjust these constants in `spi_driver.h` to match your system:

```c
#define SPI_TIMEOUT_MS      100U    // Blocking timeout (milliseconds)
#define SPI_CPU_FREQ_HZ     4000000UL  // MCU clock (Hz) — change if using PLL
#define SPI_BUFFER_SIZE     64U     // Ring buffer size (must be power-of-2)
```

**Important:** If you change the system clock (e.g., enable PLL → 24 MHz), update `SPI_CPU_FREQ_HZ` so the timeout calculation remains accurate.

---

## Usage

### Blocking Mode

Use when **simplicity** is more important than concurrency. The CPU waits for each byte to complete before proceeding.

#### Single-Byte Transfer

```c
#include "spi_driver.h"

int main(void)
{
    // Initialize SPI0: Mode 0, 1 MHz (DIV4 @ 4 MHz)
    SPI_Init(SPI_MODE_0, SPI_PRESCALER_DIV4, false);
    
    // Configure CS pin (PA4) as output, de-asserted
    PORTA.OUTSET = PIN4_bm;
    PORTA.DIRSET = PIN4_bm;
    
    // Assert CS
    SPI_CS_Low(&PORTA, PIN4_bm);
    
    // Transfer one byte
    uint8_t rxByte;
    SPI_Status_t status = SPI_TransferByte(0xAB, &rxByte);
    
    // De-assert CS
    SPI_CS_High(&PORTA, PIN4_bm);
    
    if (status == SPI_OK) {
        // rxByte contains the received data
    }
    
    while (1) { }
}
```

#### Multi-Byte Buffer Transfer

```c
// Read JEDEC ID from SPI flash (command 0x9F)
const uint8_t cmd[4] = { 0x9F, 0x00, 0x00, 0x00 };
uint8_t response[4];

SPI_CS_Low(&PORTA, PIN4_bm);
SPI_Status_t status = SPI_TransferBuffer(cmd, response, 4);
SPI_CS_High(&PORTA, PIN4_bm);

if (status == SPI_OK) {
    uint8_t mfr_id = response[1];  // Manufacturer ID
    uint8_t type   = response[2];  // Memory type
    uint8_t cap    = response[3];  // Capacity
}
```

### Non-Blocking Mode

Use when the **CPU must remain free** to do other work (process UART, update display, run PID loop) while the SPI bus operates.

```c
#include "spi_driver.h"

int main(void)
{
    // Initialize
    SPI_Init(SPI_MODE_0, SPI_PRESCALER_DIV4, false);
    PORTA.OUTSET = PIN4_bm;
    PORTA.DIRSET = PIN4_bm;
    
    // Enable global interrupts (REQUIRED for non-blocking mode)
    sei();
    
    // Prepare 16-byte payload
    uint8_t txBuf[16] = { 0x10, 0x11, 0x12, /* ... */ };
    uint8_t rxBuf[16];
    
    // Start transfer
    SPI_CS_Low(&PORTA, PIN4_bm);
    SPI_StartTransfer(txBuf, 16);
    
    // CPU is FREE — do other work here
    while (!SPI_TransferComplete()) {
        // Process UART, update LED, sleep, etc.
        do_useful_work();
    }
    
    // Transfer done — de-assert CS and read received data
    SPI_CS_High(&PORTA, PIN4_bm);
    size_t received = SPI_ReadNonBlocking(rxBuf, 16);
    
    while (1) { }
}
```

### Flash Driver Example

The included `flash_driver.h` / `.c` demonstrates how to layer a device-specific driver on top of the reusable SPI layer.

```c
#include "flash_driver.h"

int main(void)
{
    // Flash driver calls SPI_Init internally
    Flash_Status_t st = Flash_Init(&PORTA, PIN4_bm);
    
    // Read JEDEC ID
    Flash_ID_t id;
    Flash_ReadID(&id);  // id.manufacturer_id = 0xEF (Winbond)
    
    // Erase a 4 KB sector
    Flash_SectorErase(0x001000);  // Erase sector at address 0x1000
    
    // Write a page (256 bytes)
    uint8_t data[256] = { /* ... */ };
    Flash_PageWrite(0x001000, data, 256);
    
    // Read back
    uint8_t verify[256];
    Flash_Read(0x001000, verify, 256);
    
    while (1) { }
}
```

---

## API Reference

### Initialization

```c
void SPI_Init(SPI_Mode_t mode, SPI_Prescaler_t prescaler, bool clk2x);
void SPI_Deinit(void);
```

### Chip-Select Control

```c
void SPI_CS_Low(PORT_t *port, uint8_t pin_bm);   // Assert CS (active-low)
void SPI_CS_High(PORT_t *port, uint8_t pin_bm);  // De-assert CS
```

### Blocking Transfers (with timeout)

```c
SPI_Status_t SPI_TransferByte(uint8_t data, uint8_t *rxByte);
SPI_Status_t SPI_TransferBuffer(const uint8_t *txBuf, uint8_t *rxBuf, size_t len);
```

### Non-Blocking Transfers (interrupt-driven)

```c
SPI_Status_t SPI_StartTransfer(const uint8_t *txBuf, size_t len);
bool         SPI_TransferComplete(void);
size_t       SPI_ReadNonBlocking(uint8_t *rxBuf, size_t len);
```

### Diagnostics

```c
uint32_t SPI_GetTimeoutCount(void);
void     SPI_ClearTimeoutCount(void);
```

**Full API documentation with parameter descriptions, return values, and usage notes is available in the header files.**

---

## Architecture

### Layering Model

```
┌─────────────────────────────────────────┐
│  Application / main.c                   │  ← Calls Flash_ReadID, sensor_read, etc.
└─────────────────────────────────────────┘
             ▼
┌─────────────────────────────────────────┐
│  Device Drivers                          │
│  (flash_driver, sensor_driver, etc.)    │  ← Translates commands → SPI calls
└─────────────────────────────────────────┘
             ▼
┌─────────────────────────────────────────┐
│  spi_driver.h / spi_driver.c            │  ← Hardware abstraction (reusable)
└─────────────────────────────────────────┘
             ▼
┌─────────────────────────────────────────┐
│  AVR128DA48 SPI0 Hardware               │
└─────────────────────────────────────────┘
```

### Key Design Decisions

1. **CS pin management is caller-controlled** — the SPI driver never touches CS pins. This allows one driver to serve multiple devices on different ports without code duplication.

2. **Ring buffers use power-of-2 masking** — `(index + 1) & MASK` instead of modulo division. On an 8-bit AVR, this saves ~30 cycles per byte.

3. **Blocking mode uses a cycle counter, not a hardware timer** — keeps the driver self-contained and avoids consuming a scarce timer resource.

4. **ISR reads `SPI0.DATA` first** — reading DATA clears the IF flag, which must happen before any other register access to prevent re-entrant interrupts on some silicon revisions.

5. **Race-condition-safe ISR handoff** — `s_transferActive` is set *before* enabling the interrupt, so the ISR can never finish and clear the flag before `SPI_StartTransfer` returns.

---

## Flowcharts

The `FLOWCHART.md` file contains **13 detailed Mermaid diagrams** covering:

- System-level overview (blocking vs non-blocking architecture)
- Per-function logic flows (`SPI_Init`, `SPI_TransferByte`, `SPI_StartTransfer`, ISR, etc.)
- Ring buffer state machine
- Complete non-blocking sequence diagram (caller ↔ driver ↔ hardware ↔ ISR)

**View online:** Open `FLOWCHART.md` in any Mermaid-compatible viewer (GitHub, VS Code with Mermaid extension, [Mermaid Live Editor](https://mermaid.live/)).

---

## Examples

Three complete demonstrations are provided in `main.c`:

### 1. Blocking Single-Byte Write
Writes a two-byte register command (typical sensor configuration).

```c
static void demo_blocking_single_byte(void);
```

### 2. Blocking Buffer Transfer
Reads a JEDEC ID (0x9F command + 3 response bytes) — mimics the Read ID sequence used by most SPI flash and sensor ICs.

```c
static void demo_blocking_buffer(void);
```

### 3. Non-Blocking Transfer with CPU Work
Transmits a 16-byte payload while the CPU increments a counter and toggles a debug pin. A logic analyser probe on the debug pin (PA0) shows exactly how much CPU time was reclaimed.

```c
static void demo_non_blocking(void);
```

**To run the demos:**
1. Flash `main.c` to the AVR128DA48 Curiosity Nano
2. Connect a logic analyser to PA4 (CS), PC0 (SCK), PC2 (MOSI), PC1 (MISO), and PA0 (debug toggle)
3. Power on — the three demos run sequentially on boot

---

## Performance

### Blocking Mode

| Transfer Size | Approximate Duration @ 1 MHz SPI | CPU State |
|---------------|-----------------------------------|-----------|
| 1 byte | ~10 µs | Busy-waiting |
| 16 bytes | ~160 µs | Busy-waiting |
| 256 bytes | ~2.6 ms | Busy-waiting |

### Non-Blocking Mode

| Transfer Size | Approximate Duration @ 1 MHz SPI | CPU State |
|---------------|-----------------------------------|-----------|
| 1 byte | ~10 µs | **Free for other work** |
| 16 bytes | ~160 µs | **Free for other work** |
| 256 bytes | ~2.6 ms | **Free for other work** |

**Measured on AVR128DA48 @ 4 MHz with SPI clock = 1 MHz (DIV4).**

### ISR Overhead

- **Entry/exit latency:** ~5 µs (register push/pop, ISR vector jump)
- **ISR execution time:** ~15 CPU cycles (~4 µs @ 4 MHz) per byte
- **Minimum inter-byte gap:** ISR must complete before the next SPI byte finishes (safe up to ~2 MHz SPI clock @ 4 MHz CPU)

---

## Testing

### Unit Tests (Simulated)

No simulator-based tests are currently provided (AVR128DA48 simulation support is limited). Testing is performed on real hardware.

### Hardware Tests

1. **Loopback test:** Connect MOSI (PC2) to MISO (PC1). Transmit a known pattern and verify the received bytes match.

2. **Flash memory test:** Connect a W25Q32JV or compatible SPI flash. Run the flash driver demo:
   - Read JEDEC ID → verify manufacturer ID = 0xEF (Winbond)
   - Erase a sector → verify all bytes = 0xFF
   - Write a page → read back and verify

3. **Timeout test:** Remove the flash device (or de-assert CS permanently). Call `SPI_TransferByte` and verify it returns `SPI_ERR_TIMEOUT` within 100 ms.

4. **Concurrency test:** Use the non-blocking demo with a logic analyser probe on PA0 (debug toggle). Verify the CPU toggles the pin *during* the SPI transfer (proves CPU is not blocked).

---

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork** the repository
2. **Create a feature branch:** `git checkout -b feature/my-new-feature`
3. **Commit your changes:** `git commit -am 'Add support for SPI Mode 3'`
4. **Push to the branch:** `git push origin feature/my-new-feature`
5. **Submit a pull request**

### Coding Standards

- Follow the existing style (BSD/Allman brace style, 4-space indent, 80-column soft limit)
- Document all public functions with Doxygen-style comments
- Test on real hardware before submitting

### Reporting Issues

Use the GitHub Issues tab to report bugs or request features. Include:
- AVR-Dx device pack version
- Microchip Studio version (or avr-gcc version)
- Minimal reproducible example
- Expected vs actual behavior

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

### Summary

- ✅ Free to use in commercial and open-source projects
- ✅ Modify and redistribute with attribution
- ❌ No warranty provided (use at your own risk)

---

## Acknowledgments

- **Microchip Technology Inc.** — AVR128DA48 datasheet and device pack
- **AVR Libc** — Standard library for AVR microcontrollers
- **Mermaid** — Flowchart generation tool

### Inspiration

This driver was built to demonstrate best practices for embedded systems:
- Hardware abstraction layers
- Timeout protection
- Interrupt-driven I/O
- Layered architecture
- Comprehensive documentation

### References

- [AVR128DA48 Datasheet](https://www.microchip.com/en-us/product/AVR128DA48)
- [AVR-Dx Family Reference Manual](https://ww1.microchip.com/downloads/en/DeviceDoc/AVR-Dx-Family-Data-Sheet-DS40002183B.pdf)
- [Microchip Studio Documentation](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio)

---

## Contact

**Author:** Your Name  
**Email:** your.email@example.com  
**GitHub:** [@yourusername](https://github.com/yourusername)

---

**Made with ❤️ for the AVR community**

⭐ Star this repository if you find it useful!
