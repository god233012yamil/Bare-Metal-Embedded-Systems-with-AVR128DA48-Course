/**
 * @file    main.c
 * @brief   SPI0 driver demonstration for the AVR128DA48 Curiosity Nano board.
 *
 * Demonstrates three usage patterns for the reusable SPI driver:
 *
 *   1. Blocking single-byte transfer   – e.g. writing a config register.
 *   2. Blocking multi-byte buffer      – e.g. reading an ID / data burst.
 *   3. Non-blocking (interrupt-driven) – e.g. streaming data to flash
 *                                        while the CPU does other work.
 *
 * Hardware connections assumed
 * ----------------------------
 *   SPI0 signals (PORTMUX default, AVR128DA48):
 *    PA6 = SCK  (output in master mode)
 *    PA5 = MISO (input  in master mode)
 *    PA4 = MOSI (output in master mode)
 *    PA7 = SS   (not used here; handled externally as GPIO CS)
 *
 *   Chip-select pins (configure to match your board):
 *     PC4 ? CS for device 1 (e.g. SPI Flash W25Q32 or MCP4921 DAC)
 *     PC5 ? CS for device 2 (future sensor)
 *
 *   Debug output (optional):
 *     PA0 ? Toggle pin to measure non-blocking transfer timing on
 *           a logic analyzer / oscilloscope.
 *
 * Toolchain : Atmel/Microchip Studio 7, GCC C Executable Project
 * Target    : AVR128DA48 @ 4 MHz (reset default, no PLL)
 * Pack      : AVR-Dx 2.4.286
 *
 * @author  Your Name
 * @date    2026-02-17
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "spi_driver.h"

/* =========================================================================
 * Board-level pin definitions – adjust to match your wiring
 * ========================================================================= */

/** CS pin for the first SPI device (active-low).                           */
#define CS1_PORT    PORTC
#define CS1_PIN_BM  PIN4_bm

/** CS pin for a second SPI device (reserved for future use).               */
#define CS2_PORT    PORTC
#define CS2_PIN_BM  PIN5_bm

/** Debug toggle pin – useful when probing with a logic analyzer.           */
#define DBG_PORT    PORTA
#define DBG_PIN_BM  PIN0_bm

/* =========================================================================
 * Convenience macros
 * ========================================================================= */

/** Assert CS1 (pull low).                                                   */
#define CS1_ASSERT()    SPI_CS_Low(&CS1_PORT,  CS1_PIN_BM)

/** De-assert CS1 (drive high).                                              */
#define CS1_RELEASE()   SPI_CS_High(&CS1_PORT, CS1_PIN_BM)

/** Toggle the debug pin (XOR trick to flip without read-modify-write).      */
#define DBG_TOGGLE()    (DBG_PORT.OUTTGL = DBG_PIN_BM)

/* =========================================================================
 * Private function prototypes
 * ========================================================================= */

static void board_init(void);
static void demo_blocking_single_byte(void);
static void demo_blocking_buffer(void);
static void demo_non_blocking(void);

/* =========================================================================
 * Entry point
 * ========================================================================= */

/**
 * @brief  Application entry point.
 *
 * initializes the board and SPI peripheral, then runs three successive
 * demonstrations.  In a real application you would replace these demos
 * with calls to your sensor / flash driver layer.
 */
int main(void)
{
    /* ---- 1. Board-level GPIO and SPI peripheral init -------------------- */
    board_init();

    /* Enable global interrupts – required for the non-blocking ISR demo.   */
    sei();

    /* ---- 2. Run demonstrations in sequence ------------------------------ */
    demo_blocking_single_byte();
    demo_blocking_buffer();
    demo_non_blocking();

    /* ---- 3. Idle loop ---------------------------------------------------- */
    while (1)
    {
        /* In a real project: check flags set by the ISR, service a RTOS
         * tick, enter sleep, etc.                                            */
        _delay_ms(500);
    }

    return 0;   /* unreachable, suppresses compiler warning                 */
}

/* =========================================================================
 * Private functions
 * ========================================================================= */

/**
 * @brief  Initialise GPIO directions and the SPI0 peripheral.
 *
 * - CS pins are driven high (de-asserted) before SPI is enabled to prevent
 *   spurious chip-selects during start-up.
 * - Debug toggle pin is set as output.
 * - SPI0 is initialized in SPI Mode 0 with CLK_PER/4 = 1 MHz.
 */
static void board_init(void)
{
    /* ---- CS pins: output, high (idle / de-asserted) --------------------- */
    CS1_PORT.OUTSET = CS1_PIN_BM;   /* drive high before setting direction  */
    CS1_PORT.DIRSET = CS1_PIN_BM;   /* now set as output                    */

    CS2_PORT.OUTSET = CS2_PIN_BM;
    CS2_PORT.DIRSET = CS2_PIN_BM;

    /* ---- Debug pin: output ---------------------------------------------- */
    DBG_PORT.DIRSET = DBG_PIN_BM;

    /* ---- SPI0 ----------------------------------------------------------- */
    /* Mode 0  : CPOL=0, CPHA=0 (most common sensor default)
     * DIV4    : SPI clock = 4 MHz / 4 = 1 MHz
     * clk2x   : false (no doubling)                                         */
    SPI_Init(SPI_MODE_0, SPI_PRESCALER_DIV4, false);
}

/* -------------------------------------------------------------------------- */

/**
 * @brief  Demo 1 – blocking single-byte write (e.g. write a sensor register).
 *
 * Pattern used when:
 *   - The transfer is short (1–2 bytes).
 *   - Timing is not critical.
 *   - Simplicity is preferred over concurrency.
 *
 * Example mapping to a real device:
 *   Byte 0: register address (write command)
 *   Byte 1: register value
 */
static void demo_blocking_single_byte(void)
{
    /* Typical two-byte register write: [addr | 0x00 (write bit)] [value]   */
    const uint8_t REG_ADDR  = 0x20U;   /* hypothetical config register      */
    const uint8_t REG_VALUE = 0xC3U;   /* value to write                    */

    SPI_Status_t status;
    uint8_t      dummy_rx = 0U;        /* received byte (MISO) – discarded  */

    /* Assert CS to begin the transaction.                                   */
    CS1_ASSERT();

    /* Send address byte – we don't need the MISO byte for a write.         */
    status = SPI_TransferByte(REG_ADDR, &dummy_rx);
    if (status != SPI_OK)
    {
        /* Handle error: log, set an error flag, or retry.
         * Here we just de-assert CS and return to avoid stalling.           */
        CS1_RELEASE();
        return;
    }

    /* Send data byte.                                                       */
    status = SPI_TransferByte(REG_VALUE, &dummy_rx);

    /* De-assert CS to end the transaction regardless of outcome.           */
    CS1_RELEASE();

    /* (Optional) React to a timeout or error after CS is released.         */
    if (status == SPI_ERR_TIMEOUT)
    {
        /* Could set an error flag checked in the main loop, or increment a
         * counter for telemetry.  Blocking and busy-waiting would worsen the
         * situation.                                                         */
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief  Demo 2 – blocking buffer transfer (e.g. read a device JEDEC ID).
 *
 * Pattern used when:
 *   - Multiple bytes must be exchanged atomically (CS held throughout).
 *   - The received data is needed before any other work can proceed.
 *
 * This example mimics the JEDEC-standard Read ID command (0x9F) supported by
 * most SPI flash memories and many sensor ICs.  The device returns 3 bytes:
 * Manufacturer ID, Memory Type, and Capacity.
 */
static void demo_blocking_buffer(void)
{
    /* JEDEC Read-ID command + 3 dummy bytes to clock out the response.     */
    const uint8_t txBuf[4] = { 0x9FU, 0x00U, 0x00U, 0x00U };
    uint8_t       rxBuf[4] = { 0 };

    CS1_ASSERT();

    SPI_Status_t status = SPI_TransferBuffer(txBuf, rxBuf, sizeof(txBuf));

    CS1_RELEASE();

    if (status == SPI_OK)
    {
        /* rxBuf[0] echoes the command byte (ignore it).
         * rxBuf[1] = Manufacturer ID (e.g. 0xEF for Winbond)
         * rxBuf[2] = Memory type
         * rxBuf[3] = Capacity code
         * In a real driver you would validate these and store them.         */
        volatile uint8_t mfr_id = rxBuf[1];   /* suppress "unused" warning  */
        (void)mfr_id;
    }
}

/* -------------------------------------------------------------------------- */

/**
 * @brief  Demo 3 – non-blocking (interrupt-driven) transfer.
 *
 * Pattern used when:
 *   - The transfer is long (e.g. a 64-byte sensor data burst).
 *   - The CPU should remain free to do other work (e.g. process previous
 *     data, handle a UART, update a display) while the SPI bus is busy.
 *
 * Flow:
 *   1. Prepare TX data.
 *   2. Assert CS.
 *   3. Call SPI_StartTransfer() – returns immediately (non-blocking).
 *   4. Do other useful work in a loop.
 *   5. When SPI_TransferComplete() returns true, read the received bytes
 *      and de-assert CS.
 *
 * IMPORTANT: CS must be kept asserted until SPI_TransferComplete() is true.
 */
static void demo_non_blocking(void)
{
    /* Build a 16-byte payload – could be a page-write to flash, a data
     * stream to a DAC, or a sensor command sequence.                        */
    uint8_t txBuf[16];
    uint8_t rxBuf[16];
    uint8_t work_counter = 0U;   /* simulates independent CPU work           */

    for (uint8_t i = 0U; i < sizeof(txBuf); i++)
    {
        txBuf[i] = (uint8_t)(0x10U + i);   /* dummy payload: 0x10, 0x11 …   */
    }

    /* Assert CS and kick off the transfer.                                  */
    CS1_ASSERT();

    SPI_Status_t status = SPI_StartTransfer(txBuf, sizeof(txBuf));
    if (status != SPI_OK)
    {
        /* Failed to start (busy, bad args, etc.) – release CS immediately.  */
        CS1_RELEASE();
        return;
    }

    /* ---- Perform other work while the ISR drives the bus ---------------- */
    while (!SPI_TransferComplete())
    {
        /* Toggle the debug pin on each "tick" so a logic analyzer can show
         * that the CPU is not idle during the SPI transfer.                 */
        DBG_TOGGLE();

        /* Simulate work: increment a counter, process a queue, sleep, etc. */
        work_counter++;

        /* A small delay to avoid hammering SPI_TransferComplete() too hard.
         * In production code this would be replaced by real useful work.    */
        _delay_us(10);
    }

    /* ---- Transfer done – de-assert CS and collect received data --------- */
    CS1_RELEASE();

    size_t bytes_read = SPI_ReadNonBlocking(rxBuf, sizeof(rxBuf));

    /* Use bytes_read and rxBuf as needed.  work_counter tells us how many
     * "ticks" of independent work we managed during the SPI transfer.       */
    (void)bytes_read;
    (void)work_counter;

    /* Blink the debug pin once to signal completion.                        */
    DBG_TOGGLE();
    _delay_ms(1);
    DBG_TOGGLE();
}