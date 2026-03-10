/**
 * @file board.h
 * @brief Board-level configuration for AVR128DA48 Curiosity Nano demo.
 *
 * This file holds:
 * - CPU clock definition (F_CPU)
 * - LED pin mapping
 * - TWI (I2C) demo target device settings
 * - Optional internal pull-up configuration for SDA/SCL pins
 *
 * Adjust these definitions to match your wiring and device choice.
 */

#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>
#include <stdint.h>

/* CPU clock */
#ifndef F_CPU
#define F_CPU (24000000UL)
#endif

/* LED (demo heartbeat/status) */
#define LED_PORT        PORTA
#define LED_PIN_bm      PIN2_bm

/* Demo I2C target (7-bit address) */
#define DEMO_I2C_ADDR       (0x48u)

/* Demo register address (device-specific) */
#define DEMO_I2C_REG        (0x00u)

/* Number of bytes to read */
#define DEMO_I2C_READ_LEN   (2u)

/*
 * Optional internal pull-ups for SDA/SCL.
 * Update the pin controls to match your actual SDA/SCL pins.
 */
#define TWI0_SDA_PORT    PORTA
#define TWI0_SCL_PORT    PORTA
#define TWI0_SDA_PINCTRL PIN2CTRL
#define TWI0_SCL_PINCTRL PIN3CTRL

/**
 * @brief Initialize the LED GPIO.
 */
static inline void board_led_init(void)
{
    /* Configure LED pin as output */
    LED_PORT.DIRSET = LED_PIN_bm;
    /* Default LED off */
    LED_PORT.OUTCLR = LED_PIN_bm;
}

/**
 * @brief Toggle the LED GPIO.
 */
static inline void board_led_toggle(void)
{
    /* Toggle LED output */
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/**
 * @brief Initialize I2C pins (optional internal pull-ups).
 */
static inline void board_init_i2c_pins(void)
{
    /* Enable internal pull-ups (use external pull-ups when possible) */
    TWI0_SDA_PORT.TWI0_SDA_PINCTRL |= PORT_PULLUPEN_bm;
    TWI0_SCL_PORT.TWI0_SCL_PINCTRL |= PORT_PULLUPEN_bm;
}

#endif /* BOARD_H */
