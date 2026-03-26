/**
 * @file    hal.h
 * @brief   Hardware Abstraction Layer (HAL) for AVR128DA48 Curiosity Nano.
 *
 * Centralises all hardware-specific details so that the FSM application layer
 * (vending.c / main.c) never touches registers directly.
 *
 * -------------------------------------------------------------------------
 * Board pin assignment
 * -------------------------------------------------------------------------
 *
 *  Signal          MCU pin   Direction   Notes
 *  --------------- --------- ----------- ---------------------------------
 *  LED0  (green)   PC6       Output      Active LOW  – onboard LED
 *  SW0   (button)  PC7       Input       Active LOW  – onboard button,
 *                                        internal pull-up, falling-edge ISR
 *  COIN  button    PA2       Input       Active LOW  – external button,
 *                                        internal pull-up, falling-edge ISR
 *                                        (simulates coin insert)
 *  DISPENSE btn    PA3       Input       Active LOW  – external button,
 *                                        internal pull-up, falling-edge ISR
 *                                        (simulates product selection)
 *  ADC sensor      PD3/AIN3  Analog in   Potentiometer 0-VDD
 *                                        (simulates temperature / fault sensor)
 *  USART0 TX       PA0       Output      Debug UART 9600 baud
 *  USART0 RX       PA1       Input       (optional – for future commands)
 *
 * -------------------------------------------------------------------------
 * Timekeeping
 * -------------------------------------------------------------------------
 *  TCB0 fires every 1 ms (system tick).  hal_tick_ms() returns the elapsed
 *  milliseconds since power-on.  Use hal_elapsed_ms() for timeout checks.
 *
 * -------------------------------------------------------------------------
 * Author  : FSM Demo Project
 * Target  : AVR128DA48 Curiosity Nano
 * Toolchain: Atmel Studio 7 / avr-gcc
 * -------------------------------------------------------------------------
 */

#ifndef HAL_H_
#define HAL_H_

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * Initialisation
 * ========================================================================= */

/**
 * @brief Initialise all hardware peripherals.
 *
 * Must be called once at startup, before interrupts are enabled.
 * Configures: GPIO, TCB0 (1 ms tick), ADC0, USART0.
 */
void hal_init(void);

/* =========================================================================
 * LED
 * ========================================================================= */

/** @brief Turn LED0 on  (drive PC6 low). */
void hal_led_on(void);

/** @brief Turn LED0 off (drive PC6 high). */
void hal_led_off(void);

/** @brief Toggle LED0 state. */
void hal_led_toggle(void);

/* =========================================================================
 * Buttons (debounced, edge events)
 * ========================================================================= */

/**
 * @brief Button identifiers.
 *
 * HAL_BTN_SW0      – Onboard SW0 on PC7  (used as "cancel / refund")
 * HAL_BTN_COIN     – External button on PA2 (simulate coin insert)
 * HAL_BTN_DISPENSE – External button on PA3 (simulate product select)
 */
typedef enum
{
    HAL_BTN_SW0      = 0,
    HAL_BTN_COIN     = 1,
    HAL_BTN_DISPENSE = 2,
    HAL_BTN_COUNT           /**< Total number of buttons – keep last */
} hal_btn_id_t;

/**
 * @brief Poll a button and return true once per press (edge-detected).
 *
 * Implements a simple 20 ms software debounce.  Call this function
 * periodically from the main loop.
 *
 * @param[in] btn  Button to query.
 * @return    true on the rising-edge of a confirmed press, false otherwise.
 */
bool hal_btn_pressed(hal_btn_id_t btn);

/* =========================================================================
 * ADC
 * ========================================================================= */

/**
 * @brief Start a single ADC conversion on AIN3 (PD3).
 *
 * Non-blocking.  Poll hal_adc_ready() or wait for the RESRDY interrupt.
 */
void hal_adc_start(void);

/**
 * @brief Check whether an ADC conversion result is available.
 *
 * Cleared automatically when hal_adc_read() is called.
 *
 * @return true if a result is waiting.
 */
bool hal_adc_ready(void);

/**
 * @brief Read the latest 12-bit ADC result (0–4095).
 *
 * Clears the ready flag so the next call to hal_adc_ready() returns false
 * until a new conversion is started and completed.
 *
 * @return 12-bit ADC sample.
 */
uint16_t hal_adc_read(void);

/* =========================================================================
 * System tick (TCB0, 1 ms period)
 * ========================================================================= */

/**
 * @brief Return the system tick counter value in milliseconds.
 *
 * The counter wraps at 2^32 – 1 (approximately 49.7 days).
 *
 * @return Milliseconds elapsed since hal_init() was called.
 */
uint32_t hal_tick_ms(void);

/**
 * @brief Return the number of milliseconds elapsed since @p start_ms.
 *
 * Handles the 32-bit wrap-around correctly.
 *
 * @param[in] start_ms  Snapshot taken with hal_tick_ms() in the past.
 * @return    Milliseconds elapsed.
 */
uint32_t hal_elapsed_ms(uint32_t start_ms);

/* =========================================================================
 * UART debug output
 * ========================================================================= */

/**
 * @brief Transmit a single character over USART0.
 *
 * Blocks until the transmit data register is empty.
 *
 * @param[in] c  Character to send.
 */
void hal_uart_putc(char c);

/**
 * @brief Transmit a null-terminated string over USART0.
 *
 * Calls hal_uart_putc() for each character.
 *
 * @param[in] s  Pointer to the string.
 */
void hal_uart_puts(const char *s);

/**
 * @brief Transmit a 16-bit unsigned integer as a decimal string over USART0.
 *
 * @param[in] value  Value to print.
 */
void hal_uart_put_u16(uint16_t value);

#endif /* HAL_H_ */
