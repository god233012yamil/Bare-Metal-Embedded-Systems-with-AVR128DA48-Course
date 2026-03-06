/**
 * @file spi_driver.h
 * @brief SPI0 master driver (transaction-based, timeout-protected) for AVR128DA48.
 *
 * Design goals:
 * - Keep register access in the driver (application never touches SPI registers)
 * - Provide a transaction API that asserts CS for the full transfer
 * - Support multiple devices (different CS pins, modes, prescalers)
 * - Add timeout protection to avoid infinite blocking
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------- */
/* Status / Errors               */
/* ----------------------------- */
typedef enum
{
    SPI_OK = 0,
    SPI_ERR_BAD_PARAM = 1,
    SPI_ERR_TIMEOUT = 2,
    SPI_ERR_BUSY = 3
} spi_status_t;

/* ----------------------------- */
/* Driver API                    */
/* ----------------------------- */

/**
 * @brief Initialize SPI0 GPIO and SPI0 peripheral.
 *
 * Sets MOSI/SCK outputs, MISO input, configures CS pins as outputs (inactive high),
 * and enables SPI0 in master mode.
 *
 * @return SPI_OK on success.
 */
spi_status_t spi0_init(void);

/**
 * @brief Begin a transaction with a specific device.
 *
 * Applies the per-device SPI mode and prescaler and asserts chip select.
 *
 * @param dev Device ID (SPI_DEV0..).
 * @return SPI_OK or error.
 */
spi_status_t spi0_begin(spi_device_id_t dev);

/**
 * @brief End the active transaction (deassert chip select).
 *
 * @param dev Device ID (SPI_DEV0..).
 * @return SPI_OK or error.
 */
spi_status_t spi0_end(spi_device_id_t dev);

/**
 * @brief Transfer a single byte (full duplex) with timeout.
 *
 * Requires an active transaction (spi0_begin called).
 *
 * @param tx Byte to transmit.
 * @param rx Pointer to store received byte (may be NULL to ignore).
 * @param timeout_loops Simple loop timeout (0 disables timeout protection).
 * @return SPI_OK or error.
 */
spi_status_t spi0_transfer_byte(uint8_t tx, uint8_t *rx, uint32_t timeout_loops);

/**
 * @brief Transfer a buffer (full duplex) with timeout.
 *
 * Requires an active transaction (spi0_begin called).
 * If tx_buf is NULL, 0xFF filler is sent.
 * If rx_buf is NULL, received bytes are discarded.
 *
 * @param tx_buf Optional transmit buffer.
 * @param rx_buf Optional receive buffer.
 * @param len Number of bytes.
 * @param timeout_loops Timeout per byte (simple loop counter).
 * @return SPI_OK or error.
 */
spi_status_t spi0_transfer_buf(const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len, uint32_t timeout_loops);

/**
 * @brief Convenience function: full transaction wrapper.
 *
 * Asserts CS, performs transfer, deasserts CS. Applies device settings.
 *
 * @param dev Device ID.
 * @param tx_buf Optional transmit buffer (NULL -> 0xFF filler).
 * @param rx_buf Optional receive buffer (NULL -> discard).
 * @param len Number of bytes.
 * @param timeout_loops Timeout per byte.
 * @return SPI_OK or error.
 */
spi_status_t spi0_transaction(spi_device_id_t dev, const uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len, uint32_t timeout_loops);

#ifdef __cplusplus
}
#endif

#endif /* SPI_DRIVER_H */
