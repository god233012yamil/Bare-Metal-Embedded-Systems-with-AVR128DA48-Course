/**
 * @file i2c_devices.h
 * @brief Layer 3 - I2C Device Driver APIs for AVR128DA48 Demo
 *
 * Provides high-level, device-oriented functions for:
 *   - A generic I2C register read/write helper
 *   - 24Cxx series I2C EEPROM page write
 *   - DS1307/DS3231 compatible RTC time read
 *
 * These functions block internally (using twi_master_wait) and return
 * a TWI_Result_t.  Application code should use ONLY these APIs and
 * never access TWI registers directly.
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

#ifndef I2C_DEVICES_H_
#define I2C_DEVICES_H_

#include <stdint.h>
#include "twi_master.h"

/* -----------------------------------------------------------------------
 * 7-bit I2C addresses (adjust if your hardware uses different addresses)
 * ----------------------------------------------------------------------- */
#define I2C_ADDR_EEPROM   0x50u   /**< 24Cxx EEPROM base address (A2:A0 = 0) */
#define I2C_ADDR_RTC      0x68u   /**< DS1307 / DS3231 RTC address */

/* -----------------------------------------------------------------------
 * RTC time structure
 * ----------------------------------------------------------------------- */
typedef struct
{
    uint8_t seconds;    /**< Seconds  0-59 (BCD) */
    uint8_t minutes;    /**< Minutes  0-59 (BCD) */
    uint8_t hours;      /**< Hours    0-23 (BCD, 24-hour mode) */
    uint8_t day;        /**< Day of week 1-7 */
    uint8_t date;       /**< Date     1-31 (BCD) */
    uint8_t month;      /**< Month    1-12 (BCD) */
    uint8_t year;       /**< Year     0-99 (BCD, offset from 2000) */
} RTC_Time_t;

/* -----------------------------------------------------------------------
 * Public API – Layer 3
 * ----------------------------------------------------------------------- */

/**
 * @brief Write a single byte to a slave register (write: [addr][reg][val]).
 * @param slave_addr  7-bit I2C slave address.
 * @param reg         Internal register address on the slave device.
 * @param value       Byte value to write.
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t sensor_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t value);

/**
 * @brief Read one or more bytes from a slave register.
 *        Uses a combined write (register pointer) then read transaction.
 * @param slave_addr  7-bit I2C slave address.
 * @param reg         Starting register address to read from.
 * @param buf         Buffer to store received bytes.
 * @param len         Number of bytes to read (1..TWI_MAX_BUFFER_LEN-1).
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t sensor_read_reg(uint8_t slave_addr, uint8_t reg,
                              uint8_t *buf, uint8_t len);

/**
 * @brief Write a page of data to a 24Cxx I2C EEPROM.
 *        Maximum page size is device-dependent; this driver assumes 8 bytes.
 * @param mem_addr  16-bit word address within the EEPROM.
 * @param data      Pointer to data to write.
 * @param len       Number of bytes (must not exceed the page boundary).
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t eeprom_write_page(uint16_t mem_addr, const uint8_t *data,
                                uint8_t len);

/**
 * @brief Read a sequence of bytes from a 24Cxx I2C EEPROM.
 * @param mem_addr  16-bit word address to start reading from.
 * @param buf       Buffer to store received bytes.
 * @param len       Number of bytes to read.
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t eeprom_read_bytes(uint16_t mem_addr, uint8_t *buf, uint8_t len);

/**
 * @brief Read the current time from a DS1307/DS3231 RTC.
 * @param time  Pointer to an RTC_Time_t structure to populate.
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t rtc_read_time(RTC_Time_t *time);

/**
 * @brief Set the time on a DS1307/DS3231 RTC.
 * @param time  Pointer to an RTC_Time_t structure with values to write.
 * @return TWI_RESULT_OK on success, error code otherwise.
 */
TWI_Result_t rtc_set_time(const RTC_Time_t *time);

/**
 * @brief Probe the I2C bus for a device at the given address.
 *        Sends address only (zero-length write) and checks for ACK.
 * @param slave_addr  7-bit address to probe.
 * @return TWI_RESULT_OK if device acknowledged, TWI_RESULT_NACK_ADDR if absent.
 */
TWI_Result_t i2c_probe(uint8_t slave_addr);

#endif /* I2C_DEVICES_H_ */
