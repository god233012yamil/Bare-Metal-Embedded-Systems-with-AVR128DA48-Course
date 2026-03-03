# AVR128DA48 Interrupt-Driven USART with Ring Buffers

## Project Overview
This advanced project demonstrates interrupt-driven USART0 communication using ring buffers on the AVR128DA48 Curiosity Nano board. Unlike polling methods, this approach allows the CPU to perform other tasks while serial communication happens automatically in the background.

## Key Features
- ✅ **Non-blocking communication** - CPU free to do other work
- ✅ **Ring buffers** - Efficient FIFO data management for TX and RX
- ✅ **Interrupt-driven** - Automatic data transmission and reception
- ✅ **Command processing** - Interactive command-line interface
- ✅ **printf support** - Formatted output similar to standard printf
- ✅ **Buffer overflow protection** - Safe handling of buffer full conditions
- ✅ **Thread-safe** - Proper interrupt handling for shared resources

## Hardware Requirements
- **Board**: AVR128DA48 Curiosity Nano
- **Debugger**: Integrated nEDBG (provides USB-to-Serial)
- **USB Cable**: USB Type-A to Micro-B

## Software Requirements
- **IDE**: Atmel Studio 7 or Microchip Studio
- **Device Pack**: AVR-Dx 2.4.286 or newer
- **Terminal**: Serial terminal (PuTTY, Tera Term, etc.)

## Architecture Overview

### Ring Buffers
Ring buffers (circular buffers) are FIFO data structures that provide:
- **Fixed memory usage** - No dynamic allocation
- **Efficient operation** - O(1) insert and remove
- **Wrap-around** - Reuses space without data movement
- **Full/empty detection** - Clear states without data loss

```
Empty Buffer:          Partially Filled:       Full Buffer:
head = tail            head != tail            (tail+1) % size == head
                                               
 [   |   |   |   ]     [ D | D | D |   ]      [ D | D | D | D ]
  ^                      ^           ^           ^   ^
  head/tail              head        tail        head tail
```

### Interrupt Service Routines (ISRs)

**RXC (Receive Complete) ISR:**
- Triggered when a byte is received
- Reads data from USART hardware
- Stores in RX ring buffer
- If buffer full, data is discarded

**DRE (Data Register Empty) ISR:**
- Triggered when TX register is ready
- Retrieves byte from TX ring buffer
- Sends via USART hardware
- Disables itself when buffer is empty

### Data Flow

```
User Application
      |
      | write()
      v
  TX Ring Buffer  <-- DRE ISR reads and transmits
      |
      v
  USART Hardware
      |
      v
  [Physical Line]
      |
      v
  USART Hardware
      |
      v
  RX Ring Buffer  <-- RXC ISR receives and stores
      |
      | read()
      v
User Application
```

## File Structure

```
Project/
├── ring_buffer.h           - Generic ring buffer implementation
├── usart0_interrupt.h      - USART driver header
├── usart0_interrupt.c      - USART driver implementation
└── main_interrupt.c        - Main application with examples
```

### ring_buffer.h
Generic ring buffer module with:
- `ring_buffer_init()` - Initialize buffer
- `ring_buffer_put()` - Add byte (non-blocking)
- `ring_buffer_get()` - Remove byte (non-blocking)
- `ring_buffer_is_full()` - Check full state
- `ring_buffer_is_empty()` - Check empty state
- `ring_buffer_available()` - Get bytes available
- `ring_buffer_free_space()` - Get free space

### usart0_interrupt.h/.c
USART0 driver with interrupt support:
- `USART0_init()` - Initialize USART with interrupts
- `USART0_write()` - Write byte (non-blocking)
- `USART0_read()` - Read byte (non-blocking)
- `USART0_writeString()` - Write string (non-blocking)
- `USART0_writeString_blocking()` - Write string (blocking)
- `USART0_printf()` - Formatted output
- `USART0_available()` - Bytes available to read
- `USART0_txFreeSpace()` - TX buffer free space
- `USART0_txComplete()` - Check if TX finished
- ISRs for RXC and DRE

### main_interrupt.c
Example application demonstrating:
- Command-line interface
- LED control
- Background tasks while communicating
- Proper usage of interrupt-driven USART

## Configuration

### Buffer Sizes
Adjust in `usart0_interrupt.h`:
```c
#define USART0_RX_BUFFER_SIZE 128   // Receive buffer
#define USART0_TX_BUFFER_SIZE 128   // Transmit buffer
```

**Guidelines:**
- Larger buffers = more RAM usage but less overflow risk
- RX buffer should handle burst data reception
- TX buffer size depends on your output patterns
- Powers of 2 are traditional but not required

### Baud Rate
Change in `usart0_interrupt.c` `USART0_init()`:
```c
USART0.BAUD = (uint16_t)USART0_BAUD_RATE(115200);  // For 115200
```

### Clock Frequency
If modifying system clock, update F_CPU in both files:
```c
#define F_CPU 24000000UL  // For 24 MHz
```

## Project Setup

### 1. Create New Project
1. File → New → Project
2. Select "GCC C Executable Project"
3. Name: "AVR128DA48_USART_Interrupt"
4. Device: AVR128DA48

### 2. Add Source Files
Add all files to project:
- `ring_buffer.h`
- `usart0_interrupt.h`
- `usart0_interrupt.c`
- `main_interrupt.c`

### 3. Configure Build
Project Properties → Toolchain → AVR/GNU C Compiler → Optimization:
- Set to `-O1` or `-O2` for better code efficiency
- `-Os` for size optimization

### 4. Configure Programmer
Project Properties → Tool:
- Select "nEDBG"
- Interface: UPDI

### 5. Build
Build → Build Solution (F7)

## Usage

### 1. Program Device
Debug → Start Without Debugging (Ctrl+Alt+F5)

### 2. Open Terminal
Configure terminal at 9600 baud, 8N1

### 3. Expected Output
```
========================================
  AVR128DA48 Interrupt-Driven USART
  Ring Buffer Implementation
========================================

  Device: AVR128DA48
  Clock: 4000000 Hz
  Baud Rate: 9600 bps
  RX Buffer: 128 bytes
  TX Buffer: 128 bytes

Type 'help' for available commands
----------------------------------------
> 
```

### 4. Available Commands

| Command      | Description                |
|--------------|----------------------------|
| `help` or `?`| Show help menu            |
| `led on`     | Turn LED on               |
| `led off`    | Turn LED off              |
| `led toggle` | Toggle LED state          |
| `status`     | Show system status        |
| `echo <text>`| Echo back text            |
| `clear`      | Clear RX buffer           |

### Example Session
```
> help
=== Available Commands ===
  help or ?      - Show this help menu
  led on         - Turn LED on
  led off        - Turn LED off
  ...

> led on
LED turned ON

> status
=== System Status ===
  CPU Clock: 4000000 Hz
  RX Buffer: 0 bytes available
  TX Buffer: 123 bytes free
  TX Complete: No
=====================

> echo Hello World
Echo: Hello World

> led toggle
LED toggled
```

## Advantages Over Polling

### Polling Method
```c
// CPU is blocked waiting
while (!(USART0.STATUS & USART_DREIF_bm)) {
    ; // Waste CPU cycles doing nothing
}
USART0.TXDATAL = data;
```

**Problems:**
- CPU cannot do other work
- Inefficient use of resources
- Poor responsiveness to other events
- Difficult to handle multiple tasks

### Interrupt Method
```c
// Just add to buffer and continue
USART0_write(data);
// CPU is free! Do other work:
update_sensors();
process_data();
check_buttons();
```

**Benefits:**
- ✅ CPU free for other tasks
- ✅ Efficient resource usage
- ✅ Responsive to multiple events
- ✅ Easier to write complex applications
- ✅ Background transmission/reception
- ✅ Better real-time performance

## How Interrupts Work

### Initialization
```c
// Enable RX Complete Interrupt
USART0.CTRLA = USART_RXCIE_bm;

// Enable global interrupts (CRITICAL!)
sei();
```

### Automatic Reception
1. Byte arrives at USART hardware
2. RXC interrupt fires automatically
3. ISR reads byte, stores in ring buffer
4. Returns to main code
5. Main code reads from buffer when ready

### Automatic Transmission
1. Application calls `USART0_write(data)`
2. Data added to TX ring buffer
3. DRE interrupt enabled
4. ISR loads byte to USART hardware
5. When sent, ISR loads next byte
6. When buffer empty, ISR disables itself

## Thread Safety

The driver handles critical sections properly:

```c
bool USART0_write(uint8_t data)
{
    // Disable DRE interrupt temporarily
    USART0.CTRLA &= ~USART_DREIE_bm;
    
    // Safely access TX buffer
    result = ring_buffer_put(&tx_buffer, data);
    
    // Re-enable interrupt if needed
    if (result) {
        USART0.CTRLA |= USART_DREIE_bm;
    }
    
    return result;
}
```

This prevents race conditions between:
- Main code accessing buffers
- ISRs accessing same buffers

## Memory Usage

Approximate RAM usage:
```
RX Buffer: 128 bytes
TX Buffer: 128 bytes
RX Ring Buffer Structure: ~10 bytes
TX Ring Buffer Structure: ~10 bytes
Command Buffer: 64 bytes
Total: ~340 bytes
```

Flash usage:
```
Ring Buffer Code: ~200 bytes
USART Driver: ~800 bytes
ISRs: ~100 bytes
Main Application: ~1500 bytes
Total: ~2.6 KB (plenty of room on 128KB device)
```

## Troubleshooting

### No Output
1. Verify `sei()` is called after `USART0_init()`
2. Check global interrupts are enabled
3. Verify correct COM port and baud rate
4. Check USB connection

### Garbled Output
1. Verify baud rate matches (9600)
2. Check F_CPU matches actual clock
3. Verify terminal settings (8N1)

### Data Loss
1. Increase buffer sizes
2. Process data faster in main loop
3. Add flow control if needed
4. Check for buffer overflow indicators

### Characters Missing
1. Check RX buffer size
2. Verify main loop reads frequently enough
3. Add overflow detection code

### Compilation Errors
1. Ensure all files are added to project
2. Check include paths
3. Verify AVR-Dx device pack installed
4. Check C99 mode enabled

## Advanced Topics

### Buffer Size Selection

**RX Buffer:**
- Consider maximum burst length
- Account for processing latency
- Add safety margin

Example: If receiving packets of 64 bytes at 9600 baud with 50ms processing delay:
- 9600 baud ≈ 960 bytes/sec
- 50ms delay = 48 bytes potential
- 64 byte packet + 48 safety = 112 minimum
- Recommendation: 128 bytes

**TX Buffer:**
- Consider output burst patterns
- Account for transmission time
- Balance memory vs. blocking risk

### Flow Control

For high-speed or critical applications, implement hardware flow control:

```c
// Configure RTS/CTS pins
PORTA.DIRSET = PIN2_bm;  // RTS output
PORTA.DIRCLR = PIN3_bm;  // CTS input

// Check CTS before transmitting
if (!(PORTA.IN & PIN3_bm)) {
    USART0_write(data);
}

// Set RTS when buffer nearly full
if (ring_buffer_free_space(&rx_buffer) < 16) {
    PORTA.OUTSET = PIN2_bm;  // Signal "not ready"
}
```

### Error Handling

Add error detection:

```c
ISR(USART0_RXC_vect)
{
    // Check for errors first
    if (USART0.RXDATAH & (USART_FERR_bm | USART_PERR_bm | USART_BUFOVF_bm)) {
        // Handle error - read and discard
        uint8_t dummy = USART0.RXDATAL;
        error_count++;
        return;
    }
    
    uint8_t data = USART0.RXDATAL;
    
    if (!ring_buffer_put(&rx_buffer, data)) {
        // Software buffer overflow
        overflow_count++;
    }
}
```

### DMA Alternative

For very high-speed applications, consider DMA:
- Hardware handles transfers
- Zero CPU overhead
- More complex setup
- Limited availability

### Multiple USARTs

To use USART1 or USART2:
1. Copy driver files
2. Rename to usart1_interrupt.c/h
3. Change all USART0 to USART1
4. Update ISR vectors (USART1_RXC_vect, etc.)
5. Update pin configuration

## Best Practices

1. **Always enable global interrupts** after init:
   ```c
   USART0_init();
   sei();  // CRITICAL!
   ```

2. **Check return values**:
   ```c
   if (!USART0_write(data)) {
       // Handle buffer full
   }
   ```

3. **Size buffers appropriately**:
   - Start with 128 bytes
   - Monitor usage with `USART0_available()`
   - Adjust based on requirements

4. **Process data regularly**:
   ```c
   while (USART0_available()) {
       USART0_read(&data);
       process_data(data);
   }
   ```

5. **Use blocking writes for critical data**:
   ```c
   USART0_writeString_blocking("CRITICAL: ");
   ```

## Performance Metrics

At 9600 baud:
- **Byte time**: ~1.04 ms per byte
- **ISR overhead**: ~10 µs per byte
- **CPU utilization**: ~1% for serial I/O
- **Latency**: <2 ms typically

At 115200 baud:
- **Byte time**: ~87 µs per byte
- **ISR overhead**: ~10 µs per byte
- **CPU utilization**: ~11% for serial I/O
- **Latency**: <200 µs typically

## References

- **AVR128DA48 Datasheet**: [Microchip Website](https://www.microchip.com/en-us/product/AVR128DA48)
- **USART Documentation**: AVR-Dx datasheet Chapter 27
- **Interrupt System**: AVR-Dx datasheet Chapter 13
- **Curiosity Nano User Guide**: Available on Microchip website

## License
This is example code for educational purposes. Free to use and modify.
