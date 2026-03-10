/**
 * @file i2c_devices.c
 * @brief Layer 3 - I2C Device Driver Implementation for AVR128DA48 Demo
 *
 * All functions in this file build TWI_Transaction_t descriptors and
 * submit them to the Layer 2 engine via twi_master_submit() /
 * twi_master_wait().  No TWI register is accessed here.
 *
 * Target:    AVR128DA48
 * Toolchain: Atmel/Microchip Studio 7, avr-gcc
 * Pack:      AVR-Dx Device Pack 2.4.286
 */

// #include "include/i2c_devices.h"
// #include "include/twi_master.h"

#include <i2c_devices.h>
#include <twi_master.h>

#include <string.h>   /* memcpy */
#include <util/delay.h>

/* Internal scratch buffers used to build up composite Tx payloads.
 * These are static so they persist for the lifetime of the (blocking) call. */
static uint8_t s_tx_buf[TWI_MAX_BUFFER_LEN];  /**< Transmit scratch buffer */
static uint8_t s_rx_buf[TWI_MAX_BUFFER_LEN];  /**< Receive scratch buffer  */

/* -----------------------------------------------------------------------
 * Layer 3 API implementation
 * ----------------------------------------------------------------------- */

/**
 * @brief Write a single byte to a slave register.
 */
TWI_Result_t sensor_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t value)
{
    /* Pack: [register address] [data byte] */
    s_tx_buf[0] = reg;     /* First byte selects the internal register */
    s_tx_buf[1] = value;   /* Second byte is the value to write        */

    TWI_Transaction_t t = {
        .address  = slave_addr,
        .tx_buf   = s_tx_buf,
        .tx_len   = 2u,        /* Send register + value */
        .rx_buf   = NULL,
        .rx_len   = 0u,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    return twi_master_wait();  /* Block until transaction completes */
}

/**
 * @brief Read one or more bytes from a slave register (combined transaction).
 */
TWI_Result_t sensor_read_reg(uint8_t slave_addr, uint8_t reg,
                              uint8_t *buf, uint8_t len)
{
    s_tx_buf[0] = reg;   /* Write register address pointer first */

    TWI_Transaction_t t = {
        .address  = slave_addr,
        .tx_buf   = s_tx_buf,
        .tx_len   = 1u,        /* One byte: register pointer */
        .rx_buf   = buf,       /* Caller-supplied receive buffer */
        .rx_len   = len,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    return twi_master_wait();
}

/**
 * @brief Write a page of data to a 24Cxx I2C EEPROM.
 */
TWI_Result_t eeprom_write_page(uint16_t mem_addr, const uint8_t *data,
                                uint8_t len)
{
    /* Guard against buffer overflow */
    if (len > (TWI_MAX_BUFFER_LEN - 2u))
        return TWI_RESULT_BUS_ERROR;   /* Reuse error code for simplicity */

    /* 24Cxx uses a 2-byte address for devices > 256 bytes */
    s_tx_buf[0] = (uint8_t)(mem_addr >> 8u);   /* High address byte */
    s_tx_buf[1] = (uint8_t)(mem_addr & 0xFFu); /* Low  address byte */
    memcpy(&s_tx_buf[2], data, len);            /* Payload follows immediately */

    TWI_Transaction_t t = {
        .address  = I2C_ADDR_EEPROM,
        .tx_buf   = s_tx_buf,
        .tx_len   = (uint8_t)(len + 2u),  /* Address bytes + data */
        .rx_buf   = NULL,
        .rx_len   = 0u,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    r = twi_master_wait();

    /* 24Cxx needs up to 5 ms write cycle; poll ACK or simply delay */
    if (r == TWI_RESULT_OK)
        _delay_ms(5);   /* Conservative self-timed write cycle wait */

    return r;
}

/**
 * @brief Read a sequence of bytes from a 24Cxx I2C EEPROM.
 */
TWI_Result_t eeprom_read_bytes(uint16_t mem_addr, uint8_t *buf, uint8_t len)
{
    /* Set the address pointer with a write sub-transaction */
    s_tx_buf[0] = (uint8_t)(mem_addr >> 8u);
    s_tx_buf[1] = (uint8_t)(mem_addr & 0xFFu);

    TWI_Transaction_t t = {
        .address  = I2C_ADDR_EEPROM,
        .tx_buf   = s_tx_buf,
        .tx_len   = 2u,        /* Two-byte address pointer */
        .rx_buf   = buf,
        .rx_len   = len,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    return twi_master_wait();
}

/**
 * @brief Read the current time from a DS1307/DS3231 RTC.
 *        Registers 0x00-0x06 contain time data in BCD format.
 */
TWI_Result_t rtc_read_time(RTC_Time_t *time)
{
    /* DS1307: read 7 bytes starting at register 0x00 */
    TWI_Result_t r = sensor_read_reg(I2C_ADDR_RTC, 0x00u, s_rx_buf, 7u);
    if (r != TWI_RESULT_OK)
        return r;

    /* Map raw register bytes to the time structure */
    time->seconds = s_rx_buf[0] & 0x7Fu;  /* Bit 7 is oscillator enable */
    time->minutes = s_rx_buf[1];
    time->hours   = s_rx_buf[2] & 0x3Fu;  /* Mask out 12/24-hr mode bit */
    time->day     = s_rx_buf[3];
    time->date    = s_rx_buf[4];
    time->month   = s_rx_buf[5];
    time->year    = s_rx_buf[6];

    return TWI_RESULT_OK;
}

/**
 * @brief Set the time on a DS1307/DS3231 RTC.
 */
TWI_Result_t rtc_set_time(const RTC_Time_t *time)
{
    /* Build write buffer: [reg_ptr][sec][min][hr][day][date][month][year] */
    s_tx_buf[0] = 0x00u;          /* Starting register address */
    s_tx_buf[1] = time->seconds;
    s_tx_buf[2] = time->minutes;
    s_tx_buf[3] = time->hours;    /* 24-hour mode assumed */
    s_tx_buf[4] = time->day;
    s_tx_buf[5] = time->date;
    s_tx_buf[6] = time->month;
    s_tx_buf[7] = time->year;

    TWI_Transaction_t t = {
        .address  = I2C_ADDR_RTC,
        .tx_buf   = s_tx_buf,
        .tx_len   = 8u,   /* Register pointer + 7 time bytes */
        .rx_buf   = NULL,
        .rx_len   = 0u,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    return twi_master_wait();
}

/**
 * @brief Probe the I2C bus for a device at the given address.
 */
TWI_Result_t i2c_probe(uint8_t slave_addr)
{
    /* Zero-length write: just address + ACK test */
    TWI_Transaction_t t = {
        .address  = slave_addr,
        .tx_buf   = NULL,
        .tx_len   = 0u,
        .rx_buf   = NULL,
        .rx_len   = 0u,
        .callback = NULL
    };

    TWI_Result_t r = twi_master_submit(&t);
    if (r != TWI_RESULT_OK)
        return r;

    return twi_master_wait();
}
