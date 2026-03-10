/**
 * @file twi_nb.h
 * @brief Non-blocking TWI0 (I2C) master driver for AVR128DA48 (interrupt + state machine).
 *
 * Features:
 * - Non-blocking API (no busy-wait loops on WIF/RIF)
 * - ISR-driven byte transfers
 * - Supports write, read, and write-then-read with repeated START
 * - Timeout protection using a millisecond timebase (provided by user)
 * - Cooperative multitasking friendly (call twi0_task() from main loop)
 */

#ifndef TWI_NB_H
#define TWI_NB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TWI_STATE_IDLE = 0,
    TWI_STATE_START,
    TWI_STATE_ADDR_W,
    TWI_STATE_WRITE,
    TWI_STATE_REP_START,
    TWI_STATE_ADDR_R,
    TWI_STATE_READ,
    TWI_STATE_STOP,
    TWI_STATE_COMPLETE,
    TWI_STATE_ERROR
} twi_state_t;

typedef enum
{
    TWI_OK = 0,
    TWI_ERR_BAD_PARAM = 1,
    TWI_ERR_BUSY = 2,
    TWI_ERR_NACK = 3,
    TWI_ERR_BUS = 4,
    TWI_ERR_ARBLOST = 5,
    TWI_ERR_TIMEOUT = 6,
    TWI_ERR_INTERNAL = 7
} twi_status_t;

/**
 * @brief Initialize TWI0 in master mode.
 *
 * @param f_cpu_hz CPU/peripheral clock frequency in Hz.
 * @param i2c_hz Desired I2C bus speed in Hz (100k typical).
 * @return TWI_OK on success or error code.
 */
twi_status_t twi0_init(uint32_t f_cpu_hz, uint32_t i2c_hz);

/**
 * @brief Start a non-blocking write transaction.
 *
 * @param addr_7bit 7-bit I2C slave address.
 * @param tx_buf Pointer to data to write.
 * @param tx_len Number of bytes to write.
 * @param timeout_ms Timeout in milliseconds (0 disables timeout).
 * @param now_ms Current time in ms.
 * @return TWI_OK or error code.
 */
twi_status_t twi0_start_write(uint8_t addr_7bit,
                              const uint8_t *tx_buf,
                              uint16_t tx_len,
                              uint32_t timeout_ms,
                              uint32_t now_ms);

/**
 * @brief Start a non-blocking read transaction.
 *
 * @param addr_7bit 7-bit I2C slave address.
 * @param rx_buf Destination buffer.
 * @param rx_len Number of bytes to read.
 * @param timeout_ms Timeout in milliseconds (0 disables timeout).
 * @param now_ms Current time in ms.
 * @return TWI_OK or error code.
 */
twi_status_t twi0_start_read(uint8_t addr_7bit,
                             uint8_t *rx_buf,
                             uint16_t rx_len,
                             uint32_t timeout_ms,
                             uint32_t now_ms);

/**
 * @brief Start a non-blocking write-then-read transaction (repeated START).
 *
 * @param addr_7bit 7-bit I2C slave address.
 * @param tx_buf TX buffer (write phase).
 * @param tx_len TX length.
 * @param rx_buf RX buffer (read phase).
 * @param rx_len RX length.
 * @param timeout_ms Timeout in milliseconds (0 disables timeout).
 * @param now_ms Current time in ms.
 * @return TWI_OK or error code.
 */
twi_status_t twi0_start_write_read(uint8_t addr_7bit,
                                   const uint8_t *tx_buf,
                                   uint16_t tx_len,
                                   uint8_t *rx_buf,
                                   uint16_t rx_len,
                                   uint32_t timeout_ms,
                                   uint32_t now_ms);

/**
 * @brief Cooperative task function for timeout supervision.
 *
 * @param now_ms Current time in ms.
 */
void twi0_task(uint32_t now_ms);

/**
 * @brief Get current driver state.
 *
 * @return Driver state.
 */
twi_state_t twi0_get_state(void);

/**
 * @brief Get last driver status.
 *
 * @return Driver status.
 */
twi_status_t twi0_get_status(void);

/**
 * @brief Reset driver to IDLE and clear internal context.
 */
void twi0_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* TWI_NB_H */
