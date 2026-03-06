/**
 * @file    flash_driver.h
 * @brief   Skeleton SPI NOR-Flash driver layered on spi_driver.
 *
 * This file shows the pattern for building a device-specific driver that
 * sits on top of the reusable SPI layer.  The full implementation is left
 * as an exercise; the function signatures and docstrings define the contract.
 *
 * Tested against (wire-compatible devices):
 *   - Winbond W25Q32JV  (32 Mbit, 3.3 V, 4-KB sectors)
 *   - Winbond W25Q128JV (128 Mbit)
 *
 * Layering diagram:
 *
 *   +-----------------------------+
 *   |   Application / main.c     |  (uses Flash_ReadID, Flash_ReadPage …)
 *   +-----------------------------+
 *   |   flash_driver.h / .c      |  (translates commands → SPI calls)
 *   +-----------------------------+
 *   |   spi_driver.h  / .c       |  (hardware abstraction, reusable)
 *   +-----------------------------+
 *   |   AVR128DA48 SPI0 hardware  |
 *   +-----------------------------+
 *
 * @author  Your Name
 * @date    2026-02-17
 */

#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "spi_driver.h"

/* =========================================================================
 * Flash device constants (W25Q32JV defaults)
 * ========================================================================= */

/** SPI mode required by most Winbond flash devices.                         */
#define FLASH_SPI_MODE          SPI_MODE_0

/** Number of bytes in one page (minimum writable unit).                     */
#define FLASH_PAGE_SIZE_BYTES   256U

/** Number of bytes in one sector (minimum erasable unit).                   */
#define FLASH_SECTOR_SIZE_BYTES 4096U

/* =========================================================================
 * Public type definitions
 * ========================================================================= */

/**
 * @brief Identification data returned by the Read JEDEC ID command (0x9F).
 */
typedef struct
{
    uint8_t manufacturer_id;  /**< e.g. 0xEF = Winbond                      */
    uint8_t memory_type;      /**< e.g. 0x40 = W25Q series                  */
    uint8_t capacity;         /**< log2(size in bytes): 0x16 = 32 Mbit      */
} Flash_ID_t;

/**
 * @brief Flash driver status codes.
 */
typedef enum
{
    FLASH_OK           =  0,   /**< Operation completed successfully         */
    FLASH_ERR_SPI      = -1,   /**< Underlying SPI transfer failed           */
    FLASH_ERR_TIMEOUT  = -2,   /**< Device busy flag never cleared           */
    FLASH_ERR_ALIGN    = -3,   /**< Address or length alignment violation    */
    FLASH_ERR_ARG      = -4    /**< NULL pointer or invalid argument         */
} Flash_Status_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief  Initialise the flash driver and verify device presence.
 *
 * Calls SPI_Init() internally (so the caller does NOT need to call it first),
 * then issues a Read JEDEC ID command to confirm the device is responding.
 *
 * @param[in]  csPort    PORT register set for the chip-select pin.
 * @param[in]  csPinBm   Bitmask of the CS pin within csPort.
 *
 * @return FLASH_OK      – device found and responding.
 * @return FLASH_ERR_SPI – SPI transfer failed (timeout or bus error).
 */
Flash_Status_t Flash_Init(PORT_t *csPort, uint8_t csPinBm);

/**
 * @brief  Read the JEDEC manufacturer/device identification bytes.
 *
 * Issues the standard 0x9F Read-ID command and populates @p id.
 *
 * @param[out] id   Pointer to a Flash_ID_t struct to receive the result.
 *
 * @return FLASH_OK      – id populated successfully.
 * @return FLASH_ERR_SPI – SPI transfer failed.
 * @return FLASH_ERR_ARG – id pointer is NULL.
 */
Flash_Status_t Flash_ReadID(Flash_ID_t *id);

/**
 * @brief  Read @p len bytes from @p address into @p buf.
 *
 * Uses the Fast Read command (0x0B) which supports up to 104 MHz SPI clock.
 * The function may span sector boundaries without restriction.
 *
 * @param[in]  address  24-bit byte address (0 … device capacity – 1).
 * @param[out] buf      Destination buffer.  Must not be NULL.
 * @param[in]  len      Number of bytes to read (> 0).
 *
 * @return FLASH_OK      – buf populated.
 * @return FLASH_ERR_SPI – SPI error.
 * @return FLASH_ERR_ARG – buf is NULL or len == 0.
 */
Flash_Status_t Flash_Read(uint32_t address, uint8_t *buf, size_t len);

/**
 * @brief  Erase a 4 KB sector containing @p address.
 *
 * Issues Write Enable, then Sector Erase (0x20), then polls the BUSY flag
 * in the Status Register until erase completes or timeout expires.
 *
 * @note   @p address is automatically aligned down to the sector boundary.
 *
 * @param[in] address  Any address within the target sector.
 *
 * @return FLASH_OK         – sector erased.
 * @return FLASH_ERR_TIMEOUT – device remained BUSY too long.
 * @return FLASH_ERR_SPI    – SPI error.
 */
Flash_Status_t Flash_SectorErase(uint32_t address);

/**
 * @brief  Write up to FLASH_PAGE_SIZE_BYTES bytes starting at @p address.
 *
 * @p address must be page-aligned (lower 8 bits == 0) and @p len must be
 * ≤ FLASH_PAGE_SIZE_BYTES.  Issues Write Enable, then Page Program (0x02),
 * then polls BUSY.
 *
 * @note  The target page must have been erased (all 0xFF) before writing.
 *
 * @param[in] address  Page-aligned 24-bit start address.
 * @param[in] buf      Data to write.  Must not be NULL.
 * @param[in] len      Bytes to write (1 … FLASH_PAGE_SIZE_BYTES).
 *
 * @return FLASH_OK         – page programmed successfully.
 * @return FLASH_ERR_ALIGN  – address not page-aligned, or len out of range.
 * @return FLASH_ERR_ARG    – buf is NULL.
 * @return FLASH_ERR_TIMEOUT – device remained BUSY too long.
 * @return FLASH_ERR_SPI    – SPI error.
 */
Flash_Status_t Flash_PageWrite(uint32_t       address,
                               const uint8_t *buf,
                               size_t         len);

/**
 * @brief  Poll the Status Register BUSY bit until clear or timeout.
 *
 * Call this after any erase or program command if you need to block until
 * the device is ready.  For non-blocking use, call it periodically from the
 * main loop and proceed only when it returns FLASH_OK.
 *
 * @param[in] timeout_ms  Maximum number of milliseconds to wait.
 *
 * @return FLASH_OK         – device is ready (BUSY = 0).
 * @return FLASH_ERR_TIMEOUT – device still busy after timeout_ms.
 * @return FLASH_ERR_SPI    – SPI error.
 */
Flash_Status_t Flash_WaitReady(uint16_t timeout_ms);

#endif /* FLASH_DRIVER_H */
