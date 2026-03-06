/**
 * @file    spi_driver.h
 * @brief   Reusable SPI0 driver for the AVR128DA48 (AVR-Dx family).
 *
 * Provides both blocking (with timeout) and non-blocking (interrupt-driven,
 * ring-buffer-backed) transfer modes for SPI0.  The public API is designed
 * so that higher-level sensor and flash drivers can be layered on top without
 * modification.
 *
 * Hardware target  : AVR128DA48 Curiosity Nano
 * Peripheral used  : SPI0 (PORTC  – SCK=PC0, MOSI=PC2, MISO=PC1)
 * Chip-select (CS) : managed externally by the caller; helpers are provided.
 * Clock source     : CLK_PER (default 4 MHz after reset)
 * Toolchain        : Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack      : AVR-Dx 2.4.286
 *
 * Usage example (blocking):
 * @code
 *   SPI_Init(SPI_MODE_0, SPI_PRESCALER_DIV4, false);
 *   SPI_CS_Low(&PORTA, PIN4_bm);
 *   uint8_t rx = SPI_TransferByte(0xAB);
 *   SPI_CS_High(&PORTA, PIN4_bm);
 * @endcode
 *
 * Usage example (non-blocking):
 * @code
 *   SPI_Init(SPI_MODE_0, SPI_PRESCALER_DIV4, false);
 *   uint8_t txBuf[] = {0x9F, 0x00, 0x00, 0x00};
 *   uint8_t rxBuf[4];
 *   SPI_StartTransfer(txBuf, rxBuf, 4);
 *   while (!SPI_TransferComplete()) { / * do other work * / }
 * @endcode
 *
 * @author  Your Name
 * @date    2026-02-17
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

/* =========================================================================
 * Includes
 * ========================================================================= */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* =========================================================================
 * Public constants – adjust to suit your system clock and target device
 * ========================================================================= */

/** Milliseconds to wait before a blocking transfer declares a timeout.      */
#define SPI_TIMEOUT_MS          100U

/**
 * @brief Approximate CPU cycles per millisecond.
 *
 * The AVR128DA48 runs at 4 MHz after reset (no PLL, no prescaler).
 * Change this if you reconfigure CLKCTRL before calling SPI_Init().
 */
#define SPI_CPU_FREQ_HZ         4000000UL
#define SPI_CYCLES_PER_MS       (SPI_CPU_FREQ_HZ / 1000UL)

/** Size (bytes) of the non-blocking TX/RX ring buffers.  Must be a power-of-2.  */
#define SPI_BUFFER_SIZE         64U

/* =========================================================================
 * Public type definitions
 * ========================================================================= */

/**
 * @brief SPI clock polarity / phase mode (CPOL, CPHA).
 *
 * Matches the MODE[1:0] field in SPI0.CTRLB.
 */
typedef enum
{
    SPI_MODE_0 = 0x00,  /**< CPOL=0, CPHA=0 – idle low,  sample on rising edge  */
    SPI_MODE_1 = 0x01,  /**< CPOL=0, CPHA=1 – idle low,  sample on falling edge */
    SPI_MODE_2 = 0x02,  /**< CPOL=1, CPHA=0 – idle high, sample on falling edge */
    SPI_MODE_3 = 0x03   /**< CPOL=1, CPHA=1 – idle high, sample on rising edge  */
} SPI_Mode_t;

/**
 * @brief SPI clock prescaler divider values.
 *
 * Matches the PRESC[1:0] field in SPI0.CTRLA.
 * Final SPI clock = CLK_PER / divider.
 */
typedef enum
{
    SPI_PRESCALER_DIV4   = 0x00,  /**< CLK_PER / 4   (1 MHz @ 4 MHz sys clk)  */
    SPI_PRESCALER_DIV16  = 0x01,  /**< CLK_PER / 16  (250 kHz @ 4 MHz)         */
    SPI_PRESCALER_DIV64  = 0x02,  /**< CLK_PER / 64  (62.5 kHz @ 4 MHz)        */
    SPI_PRESCALER_DIV128 = 0x03   /**< CLK_PER / 128 (31.25 kHz @ 4 MHz)       */
} SPI_Prescaler_t;

/**
 * @brief Return codes used by SPI driver functions.
 */
typedef enum
{
    SPI_OK       =  0,  /**< Operation completed successfully                  */
    SPI_ERR_BUSY = -1,  /**< SPI bus is currently busy (non-blocking mode)     */
    SPI_ERR_NULL = -2,  /**< A NULL pointer was passed as a buffer argument    */
    SPI_ERR_SIZE = -3,  /**< Transfer length is 0 or exceeds SPI_BUFFER_SIZE   */
    SPI_ERR_TIMEOUT = -4 /**< Blocking transfer timed out                     */
} SPI_Status_t;

/* =========================================================================
 * Public API – initialisation
 * ========================================================================= */

/**
 * @brief  Initialise SPI0 as master on the AVR128DA48.
 *
 * Configures the SPI0 peripheral:
 *   - Master mode
 *   - Selected clock mode (polarity/phase)
 *   - Selected prescaler
 *   - Optionally enables the Clock Doubling (CLK2X) bit
 *
 * Pin directions are also configured:
 *   - PORTC PC0 (SCK)  → OUTPUT
 *   - PORTC PC2 (MOSI) → OUTPUT
 *   - PORTC PC1 (MISO) → INPUT
 *
 * Chip-select (SS) pins are NOT managed here; use SPI_CS_Low() / SPI_CS_High().
 *
 * @note  Call this function before any transfer function.
 *        Global interrupts do NOT need to be enabled for blocking mode,
 *        but MUST be enabled (sei()) before using non-blocking mode.
 *
 * @param  mode      Clock polarity/phase – SPI_MODE_0 … SPI_MODE_3.
 * @param  prescaler Clock divider        – SPI_PRESCALER_DIV4 … DIV128.
 * @param  clk2x     Set true to halve the effective prescaler
 *                   (e.g. DIV4 + clk2x = DIV2).
 */
void SPI_Init(SPI_Mode_t mode, SPI_Prescaler_t prescaler, bool clk2x);

/**
 * @brief  Disable SPI0 and tri-state its pins.
 *
 * Clears the ENABLE bit in SPI0.CTRLA.  Call this to power-gate the
 * peripheral between sensor readings if power saving is required.
 */
void SPI_Deinit(void);

/* =========================================================================
 * Public API – chip-select helpers
 * ========================================================================= */

/**
 * @brief  Assert (pull low) an active-low chip-select pin.
 *
 * The caller is responsible for supplying the correct PORT and pin mask.
 * The selected pin direction must already be OUTPUT (typically set once in
 * your board_init() function).
 *
 * @param  port     Pointer to the PORT_t register set, e.g. &PORTA.
 * @param  pin_bm   Bitmask of the pin, e.g. PIN4_bm or (1 << 4).
 *
 * @code
 *   SPI_CS_Low(&PORTA, PIN4_bm);   // Assert CS on PA4
 * @endcode
 */
void SPI_CS_Low(PORT_t *port, uint8_t pin_bm);

/**
 * @brief  De-assert (pull high) an active-low chip-select pin.
 *
 * @param  port    Pointer to the PORT_t register set.
 * @param  pin_bm  Bitmask of the pin.
 */
void SPI_CS_High(PORT_t *port, uint8_t pin_bm);

/* =========================================================================
 * Public API – blocking transfers (with timeout)
 * ========================================================================= */

/**
 * @brief  Transfer a single byte over SPI0 (blocking, with timeout).
 *
 * Writes @p data to the SPI DATA register, then spins until the IF flag
 * (interrupt / transfer-complete flag) is set or the timeout expires.
 *
 * @note  Must NOT be called while a non-blocking transfer is in progress.
 *
 * @param[in]  data    Byte to transmit.
 * @param[out] rxByte  Pointer to store the received byte.
 *                     Pass NULL if the received value is not needed.
 *
 * @return  SPI_OK          – byte exchanged successfully.
 * @return  SPI_ERR_TIMEOUT – IF flag never set within SPI_TIMEOUT_MS.
 */
SPI_Status_t SPI_TransferByte(uint8_t data, uint8_t *rxByte);

/**
 * @brief  Exchange a buffer of bytes over SPI0 (blocking, with timeout).
 *
 * Transfers @p len bytes from @p txBuf, storing received bytes in @p rxBuf.
 * A timeout is applied to each individual byte; if any byte times out the
 * function aborts and returns SPI_ERR_TIMEOUT.
 *
 * @note  Either @p txBuf or @p rxBuf (but NOT both) may be NULL:
 *        - NULL txBuf → sends 0xFF dummy bytes (read-only operation).
 *        - NULL rxBuf → discards received bytes  (write-only operation).
 *
 * @param[in]  txBuf  Transmit buffer, or NULL to send 0xFF.
 * @param[out] rxBuf  Receive buffer,  or NULL to discard.
 * @param[in]  len    Number of bytes to transfer (must be > 0).
 *
 * @return  SPI_OK          – all bytes exchanged successfully.
 * @return  SPI_ERR_NULL    – both txBuf and rxBuf are NULL.
 * @return  SPI_ERR_SIZE    – len == 0.
 * @return  SPI_ERR_TIMEOUT – timeout on at least one byte.
 */
SPI_Status_t SPI_TransferBuffer(const uint8_t *txBuf,
                                uint8_t       *rxBuf,
                                size_t         len);

/* =========================================================================
 * Public API – non-blocking (interrupt-driven) transfers
 * ========================================================================= */

/**
 * @brief  Begin a non-blocking SPI transfer.
 *
 * Copies up to @p len bytes from @p txBuf into the driver's internal TX
 * ring buffer and enables the SPI TX-Complete interrupt.  The ISR drives the
 * bus byte-by-byte; received bytes are stored in the internal RX ring buffer
 * and can be read with SPI_ReadNonBlocking() after SPI_TransferComplete()
 * returns true.
 *
 * Chip-select assertion is the caller's responsibility (call SPI_CS_Low()
 * before this function).
 *
 * @note  Global interrupts must be enabled (sei()) before calling this.
 * @note  Returns SPI_ERR_BUSY immediately if a transfer is already running.
 *
 * @param[in] txBuf  Data to transmit.  Must not be NULL.
 * @param[in] len    Number of bytes (1 … SPI_BUFFER_SIZE).
 *
 * @return  SPI_OK       – transfer started, ISR will complete it.
 * @return  SPI_ERR_BUSY – previous transfer still in progress.
 * @return  SPI_ERR_NULL – txBuf is NULL.
 * @return  SPI_ERR_SIZE – len == 0 or len > SPI_BUFFER_SIZE.
 */
SPI_Status_t SPI_StartTransfer(const uint8_t *txBuf, size_t len);

/**
 * @brief  Query whether the non-blocking transfer has finished.
 *
 * The function returns true once the ISR has exchanged all bytes that were
 * queued by SPI_StartTransfer().  After this, call SPI_ReadNonBlocking() to
 * retrieve received data, then SPI_CS_High() to de-assert the CS pin.
 *
 * @return  true  – transfer complete (or no transfer was ever started).
 * @return  false – transfer still in progress.
 */
bool SPI_TransferComplete(void);

/**
 * @brief  Read bytes received during the last non-blocking transfer.
 *
 * Should only be called after SPI_TransferComplete() returns true.
 * Drains up to @p len bytes from the internal RX ring buffer into @p rxBuf.
 *
 * @param[out] rxBuf  Destination buffer.  Must not be NULL.
 * @param[in]  len    Maximum bytes to copy.
 *
 * @return  Number of bytes actually copied (may be less than @p len if the
 *          RX buffer contains fewer bytes).
 */
size_t SPI_ReadNonBlocking(uint8_t *rxBuf, size_t len);

/* =========================================================================
 * Public API – diagnostics / status
 * ========================================================================= */

/**
 * @brief  Return the number of timeout errors recorded since the last reset.
 *
 * Useful during development to detect bus stalls without halting execution.
 *
 * @return  Cumulative timeout count (wraps at UINT32_MAX).
 */
uint32_t SPI_GetTimeoutCount(void);

/**
 * @brief  Reset the timeout counter to zero.
 */
void SPI_ClearTimeoutCount(void);

#endif /* SPI_DRIVER_H */
