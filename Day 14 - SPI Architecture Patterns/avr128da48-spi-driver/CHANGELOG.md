# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - 2026-02-17

### Added

#### Core SPI Driver
- Blocking transfer mode with cycle-counter timeout protection
- Non-blocking (interrupt-driven) transfer mode with ring buffers
- Dual-mode support: `SPI_TransferByte()` and `SPI_TransferBuffer()` for blocking
- Non-blocking API: `SPI_StartTransfer()`, `SPI_TransferComplete()`, `SPI_ReadNonBlocking()`
- Chip-select helpers: `SPI_CS_Low()` and `SPI_CS_High()` for any PORT/pin
- Four SPI modes (MODE_0 through MODE_3)
- Four clock prescaler options (DIV4, DIV16, DIV64, DIV128)
- Optional CLK2X mode for halving the effective prescaler
- Diagnostic counters: `SPI_GetTimeoutCount()` and `SPI_ClearTimeoutCount()`

#### Flash Driver (Example)
- W25Q32JV / W25Q128JV SPI NOR-Flash driver as a layering example
- JEDEC ID read (`Flash_ReadID`)
- Fast Read command (`Flash_Read`)
- Sector erase (4 KB) with busy-wait polling
- Page program (256 bytes) with alignment enforcement
- Status register polling with timeout (`Flash_WaitReady`)

#### Documentation
- Comprehensive README.md with installation, usage, and API reference
- FLOWCHART.md with 13 Mermaid diagrams covering all driver logic flows
- Doxygen-style docstrings for every public function
- Three complete demonstration programs in `main.c`:
  - Blocking single-byte transfer
  - Blocking buffer transfer (JEDEC ID read)
  - Non-blocking transfer with CPU work

#### Project Infrastructure
- MIT License
- .gitignore for Atmel/Microchip Studio and AVR toolchains
- CHANGELOG.md (this file)

### Notes

- Target: AVR128DA48 @ 4 MHz (AVR-Dx Device Pack 2.4.286)
- Tested on AVR128DA48 Curiosity Nano board
- Compatible with Microchip Studio 7 and avr-gcc command-line toolchain

---

## [Unreleased]

### Planned Features

- [ ] Support for SPI buffered mode (BUFEN bit in SPI0.CTRLB)
- [ ] DMA-based transfers for high-throughput scenarios
- [ ] Additional device driver examples (BME280 sensor, MCP4921 DAC)
- [ ] Unit tests with AVR simulator (if/when AVR-Dx simulation improves)
- [ ] Example integration with FreeRTOS

---

[1.0.0]: https://github.com/yourusername/avr128da48-spi-driver/releases/tag/v1.0.0
[Unreleased]: https://github.com/yourusername/avr128da48-spi-driver/compare/v1.0.0...HEAD
