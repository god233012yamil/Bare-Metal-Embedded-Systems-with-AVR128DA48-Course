# AVR128DA48 USART0 Polling Demonstration

## Project Overview
This project demonstrates basic USART0 serial communication using the polling method on the AVR128DA48 Curiosity Nano board.

## Hardware Requirements
- **Board**: AVR128DA48 Curiosity Nano
- **Debugger**: Integrated nEDBG on Curiosity Nano (provides USB-to-Serial bridge)
- **USB Cable**: USB Type-A to Micro-B cable

## Software Requirements
- **IDE**: Atmel Studio 7 or Microchip Studio
- **Device Pack**: AVR-Dx 2.4.286 or newer
- **Terminal**: Any serial terminal (PuTTY, Tera Term, Arduino Serial Monitor, etc.)

## USART0 Configuration
- **Baud Rate**: 9600 bps
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None
- **Clock**: 4 MHz (default internal oscillator)

## Pin Configuration
| Pin  | Function | Direction |
|------|----------|-----------|
| PA0  | TxD      | Output    |
| PA1  | RxD      | Input     |

## Project Setup in Microchip Studio

### 1. Create New Project
1. Open Microchip Studio
2. File → New → Project
3. Select "GCC C Executable Project"
4. Enter project name (e.g., "AVR128DA48_USART_Polling")
5. Click OK

### 2. Select Device
1. In the Device Selection dialog, select **AVR128DA48**
2. Click OK

### 3. Add Source Files
1. Copy the `main.c` file into your project directory
2. (Optional) Copy `usart0.h` if you want to separate the driver
3. Add files to the project in Solution Explorer

### 4. Configure Tool
1. Right-click project → Properties
2. Go to "Tool" section
3. Select "nEDBG" (should auto-detect)
4. Interface: debugWIRE or UPDI (use UPDI for AVR128DA48)

### 5. Build Project
1. Build → Build Solution (F7)
2. Verify no errors

## Programming and Testing

### 1. Program the Device
1. Connect Curiosity Nano board via USB
2. Debug → Start Without Debugging (Ctrl+Alt+F5)
   - Or click "Start Without Debugging" button

### 2. Open Serial Terminal
The Curiosity Nano has an integrated CDC (Virtual COM Port):

**Windows:**
1. Open Device Manager
2. Find "Ports (COM & LPT)"
3. Note the COM port number (e.g., COM3)
4. Open your terminal software
5. Configure: 9600 baud, 8N1, no flow control

**Linux/Mac:**
1. Terminal: `ls /dev/tty*` to find the port (e.g., /dev/ttyACM0)
2. Use screen: `screen /dev/ttyACM0 9600`
   - Or any terminal emulator

### 3. Expected Behavior
Upon reset/startup, you should see:
```
===================================
AVR128DA48 USART0 Polling Demo
===================================
Echo Mode Active
Type any character to echo it back
-----------------------------------
```

Then any character you type will be echoed back to the terminal.

## Code Structure

### main.c
The main source file contains:

1. **USART0_init()**: Initializes USART0 hardware
   - Sets baud rate to 9600
   - Enables transmitter and receiver
   - Configures frame format (8N1)
   - Sets up GPIO pins

2. **USART0_sendChar()**: Sends a single character
   - Polls DREIF flag (Data Register Empty)
   - Writes data when buffer is ready

3. **USART0_sendString()**: Sends a null-terminated string
   - Calls sendChar() repeatedly

4. **USART0_receiveChar()**: Receives a single character
   - Polls RXCIF flag (Receive Complete)
   - Returns data when available

5. **USART0_isDataAvailable()**: Non-blocking check
   - Returns 1 if data available, 0 otherwise

6. **main()**: Application entry point
   - Initializes USART0
   - Sends welcome message
   - Runs echo loop indefinitely

## Polling Method Explanation

The **polling method** means the CPU actively waits (blocks) for events:

- **Transmit**: Loop waits until `DREIF` flag is set (buffer empty)
- **Receive**: Loop waits until `RXCIF` flag is set (data available)

**Advantages:**
- Simple to implement
- Predictable behavior
- No interrupt overhead

**Disadvantages:**
- CPU is blocked while waiting
- Inefficient for low data rates
- Cannot do other tasks while waiting

**Alternatives:**
- Interrupt-driven USART (more efficient)
- DMA-based transfers (highest throughput)

## Troubleshooting

### No Output in Terminal
1. Verify correct COM port selected
2. Check baud rate is 9600
3. Ensure board is powered (LED should be on)
4. Try pressing Reset button on board
5. Check USB cable connection

### Garbled Characters
1. Verify baud rate matches (9600)
2. Check terminal settings (8N1)
3. Verify F_CPU matches actual clock (4 MHz default)

### Cannot Program Device
1. Check Tool selection in project properties
2. Verify USB connection
3. Try different USB port/cable
4. Update Device Pack in Tools → Device Pack Manager

### Build Errors
1. Verify AVR-Dx Device Pack is installed
2. Check F_CPU definition matches your setup
3. Ensure correct device selected (AVR128DA48)

## Customization

### Change Baud Rate
Modify the USART0_init() function:
```c
USART0.BAUD = (uint16_t)USART0_BAUD_RATE(115200);  // For 115200 baud
```

Common baud rates: 9600, 19200, 38400, 57600, 115200

### Change Clock Frequency
If you modify the system clock, update F_CPU:
```c
#define F_CPU 24000000UL  // For 24 MHz
```

### Use Different USART
The AVR128DA48 has multiple USARTs (USART0, USART1, USART2).
To use USART1:
- Change all `USART0` to `USART1` in code
- Update pin configuration (PC0/PC1 for USART1 default pins)

## Additional Resources

- **AVR128DA48 Datasheet**: [Microchip Website](https://www.microchip.com/en-us/product/AVR128DA48)
- **Curiosity Nano User Guide**: Search "AVR128DA48 Curiosity Nano" on Microchip site
- **AVR-Dx Family Documentation**: Available in Microchip Studio Help

## License
This is example code for educational purposes. Feel free to modify and use in your projects.
