# AVR128DA48 TWI (I2C) Driver Demo

A clean, four-layer interrupt-driven I2C master driver for the **AVR128DA48**,
demonstrated on the **AVR128DA48 Curiosity Nano** evaluation board.

---

## Architecture

```
┌──────────────────────────────────────────────────────┐
│  Layer 4 – Application  (main.c)                     │
│  Uses device APIs only; never touches TWI registers  │
├──────────────────────────────────────────────────────┤
│  Layer 3 – Device Drivers  (i2c_devices.c/h)         │
│  sensor_read_reg(), eeprom_write_page(), rtc_read_time() │
├──────────────────────────────────────────────────────┤
│  Layer 2 – Transaction Engine  (twi_master.c/h)      │
│  State machine, buffer management, error handling    │
│  Runs inside TWI0_TWIM_vect ISR                      │
├──────────────────────────────────────────────────────┤
│  Layer 1 – Hardware Control  (twi_hw.h)              │
│  Inline register access, START/STOP/ACK primitives   │
└──────────────────────────────────────────────────────┘
```

## State Machine (Layer 2)

```
IDLE ──submit()──► START ──► SEND_ADDRESS
                                │
               ┌────────────────┼─────────────────┐
            (write)           (read)          (write+read)
               │                │                  │
          WRITE_DATA        READ_DATA        WRITE_DATA
               │                │                  │
         (all bytes tx)   (last byte rx)    REPEATED_START
               │                │                  │
             STOP             STOP            READ_DATA
               │                │                  │
           COMPLETE         COMPLETE             STOP
                                                   │
                                               COMPLETE
Any state ──error──► ERROR ──► IDLE
```

## File Structure

```
AVR128DA48_TWI_Demo.atsln          ← Atmel/Microchip Studio solution
AVR128DA48_TWI_Demo/
├── AVR128DA48_TWI_Demo.cproj      ← Studio project file
├── Makefile                       ← Command-line build (avr-gcc)
├── include/
│   ├── twi_hw.h                   ← Layer 1: hardware primitives (inline)
│   ├── twi_master.h               ← Layer 2: transaction engine API
│   └── i2c_devices.h              ← Layer 3: device driver API + types
└── src/
    ├── main.c                     ← Layer 4: application demo
    ├── twi_master.c               ← Layer 2: ISR + state machine
    └── i2c_devices.c              ← Layer 3: sensor/EEPROM/RTC functions
```

## Hardware Setup

| Signal | AVR128DA48 Pin | Notes |
|--------|---------------|-------|
| SDA    | PA2           | 4.7 kΩ pull-up to 3.3 V |
| SCL    | PA3           | 4.7 kΩ pull-up to 3.3 V |
| TXD    | PC0           | USB CDC via on-board debugger |

### Optional peripherals demonstrated
| Device | Address | Demo |
|--------|---------|------|
| 24C02 EEPROM | 0x50 | Page write + read-back |
| DS1307/DS3231 RTC | 0x68 | Time read |
| LM75/TMP75 | 0x48 | Temperature read |

Connect any combination of the above; the bus scan will detect what
is present and the individual demos gracefully report failures.

## I2C Configuration

| Parameter | Value |
|-----------|-------|
| F_CPU     | 4 MHz (default internal oscillator) |
| SCL speed | 100 kHz (Standard Mode) |
| BAUD register | 15 |
| Mode      | Master, interrupt-driven |

## Building

### Atmel / Microchip Studio
1. Open `AVR128DA48_TWI_Demo.atsln`.
2. Select **Debug** or **Release** configuration.
3. Press **F7** to build.
4. Press **Start Without Debugging** (Ctrl+Alt+F5) to program and run.

### Command Line (avr-gcc)
```bash
cd AVR128DA48_TWI_Demo
make          # builds .elf and .hex
make flash    # programs via AVRDUDE (adjust PROGRAMMER in Makefile)
make clean    # removes build artefacts
```

## Observing Output

Open a serial terminal (e.g. Tera Term, PuTTY) at **9600-8-N-1** on the
virtual COM port exposed by the Curiosity Nano on-board debugger.

Sample output:
```
========================================
 AVR128DA48 TWI (I2C) Driver Demo
 Layers: HW -> Engine -> Device -> App
========================================

--- I2C Bus Scan ---
  Found device at 0x48
  Found device at 0x50
  Found device at 0x68
  Total: 3 device(s).

--- EEPROM Demo ---
  Write 'AVR-I2C': OK
  Read  'AVR-I2C': OK

--- RTC Demo ---
  Time: 12:34:56  Date: 2024-06-15

--- LM75 Temperature Demo ---
  Temperature: 23.5 C

[Waiting 5 s before next cycle...]
```

## Key Design Decisions

- **No blocking in ISR** – the ISR only updates state and fires callbacks.
- **ATOMIC_BLOCK** – used when reading volatile state from main context.
- **Combined transactions** – a single `TWI_Transaction_t` with both
  `tx_len > 0` and `rx_len > 0` automatically generates a repeated START.
- **Callback support** – optional function pointer in the transaction
  descriptor for non-blocking notification from ISR context.
- **Layered include discipline** – `main.c` includes only `i2c_devices.h`
  and `twi_master.h`; it never includes `twi_hw.h`.

## Adapting the Driver

| Change | Where |
|--------|-------|
| Different I2C speed | `TWI_BAUD_VALUE` in `twi_hw.h` |
| Different F_CPU | `F_CPU` macro in Makefile / project symbols |
| Add a new device | Add functions to `i2c_devices.c/h` only |
| Non-blocking use | Provide a `callback` in `TWI_Transaction_t`; poll `twi_master_busy()` |
| Use TWI1 | Replace `TWI0` references in `twi_hw.h` with `TWI1` |
