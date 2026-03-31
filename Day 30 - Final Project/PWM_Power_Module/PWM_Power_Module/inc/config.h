/**
 * @file    config.h
 * @brief   Global project configuration for PWM-Controlled Power Module.
 *
 * Centralities all compile-time constants, hardware pin assignments, and
 * feature-enable flags for the AVR128DA48 Curiosity Nano target.
 * Every other module includes this file instead of defining magic numbers
 * locally, keeping the code base easy to retarget.
 *
 * @author  Yamil Garcia
 * @date    2026-03-29
 * @version 1.0.0
 *
 * Target MCU : AVR128DA48
 * Clock      : 24 MHz (OSCHF, configured in main.c)
 * IDE        : Atmel Studio 7 / Microchip Studio
 * DFP        : AVR-Dx 2.4.286
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * System clock
 * ========================================================================= */

/** CPU / peripheral clock in Hz (OSCHF at 24 MHz). */
#define F_CPU_HZ    24000000UL

/* =========================================================================
 * Scheduler tick
 * The RTC PIT fires at 1 kHz (OSC32K / 32 = 1024 Hz, ≈ 1 ms).
 * All task periods are expressed in tick counts.
 * ========================================================================= */

/** Approximate tick rate (Hz). */
#define SCHED_TICK_HZ           1024U

/** ADC sample task period (ms ≈ ticks at 1024 Hz → ~50 ms). */
#define TASK_ADC_PERIOD_TICKS   51U

/** Control-loop task period ticks (~100 ms). */
#define TASK_CTRL_PERIOD_TICKS  102U

/** USART telemetry task period ticks (~500 ms). */
#define TASK_TELEM_PERIOD_TICKS 512U

/** LED heartbeat task period ticks (~500 ms). */
#define TASK_LED_PERIOD_TICKS   512U

/* =========================================================================
 * ADC configuration
 * PD2 = AIN2 on the AVR128DA48 (connected to on-board potentiometer /
 * external signal input on the Curiosity Nano).
 * ========================================================================= */

/** ADC positive mux: AIN2 (PD2). */
#define ADC_MUX_INPUT           ADC_MUXPOS_AIN2_gc

/** ADC resolution: 12-bit. */
#define ADC_RESOLUTION          ADC_RESSEL_12BIT_gc

/** ADC clock prescaler: CLK_PER/16 → 24 MHz/16 = 1.5 MHz (within 0.5–2 MHz). */
#define ADC_PRESCALER           ADC_PRESC_DIV16_gc

/** Full-scale ADC count for 12-bit mode. */
#define ADC_FULL_SCALE          4095U

/* =========================================================================
 * TCA0 PWM configuration
 * WO0 is mapped to PORTD via PORTMUX → PD0.
 * Single-slope PWM, 10-bit equivalent period for ~23 kHz.
 *   PWM_freq = F_CPU / (CLKDIV * (PER + 1))
 *            = 24 000 000 / (1 * 1024) ≈ 23.4 kHz
 * ========================================================================= */

/** TCA0 clock divider: DIV1 (no prescale). */
#define TCA0_CLKDIV             TCA_SINGLE_CLKSEL_DIV1_gc

/** TCA0 top value (PER register) → 10-bit equivalent = 1023. */
#define TCA0_PERIOD             1023U

/** Minimum duty cycle count (0 %). */
#define PWM_DUTY_MIN            0U

/** Maximum duty cycle count (100 %). */
#define PWM_DUTY_MAX            TCA0_PERIOD

/* =========================================================================
 * Analog Comparator (AC0) configuration
 * AINP0 = PD0 is used as AC positive input.
 * DACREF is used as programmable negative reference.
 * Threshold voltage ≈ VDD * (DACREF / 255).
 * ========================================================================= */

/** AC0 positive mux: AINP0 (PD2 mapped via AC mux – see datasheet §28). */
#define AC_MUX_POS              AC_MUXPOS_AINP0_gc

/** AC0 negative mux: internal DAC reference. */
#define AC_MUX_NEG              AC_MUXNEG_DACREF_gc

/**
 * AC DACREF value for threshold detection.
 * Default ≈ 50 % of VDD: DACREF = 128 → V_threshold ≈ 2.5 V @ VDD=5V.
 */
#define AC_DACREF_DEFAULT       128U

/* =========================================================================
 * USART0 configuration
 * Default pin mapping: PA0 = TXD, PA1 = RXD.
 * Curiosity Nano routes USART0 default pins to the CDC-USB bridge.
 * ========================================================================= */

/** USART baud rate (bps). */
#define USART_BAUD_RATE         115200UL

/**
 * USART BAUD register value formula (async, CLK2X=0):
 *   BAUD = (64 * F_CPU) / (16 * baud)
 */
#define USART_BAUD_REG_VALUE    ((uint16_t)((64UL * F_CPU_HZ) / (16UL * USART_BAUD_RATE)))

/** USART TX direction pin mask on PORTA (PA0). */
#define USART_TX_PIN_bm         PIN0_bm

/** USART RX direction pin mask on PORTA (PA1). */
#define USART_RX_PIN_bm         PIN1_bm

/* =========================================================================
 * FIFO / ring-buffer size
 * Must be a power of 2 for the fast modulo optimization.
 * ========================================================================= */

/** USART RX ring-buffer capacity (bytes). */
#define FIFO_RX_SIZE            64U

/* =========================================================================
 * LED (Curiosity Nano: active-low LED on PC6)
 * ========================================================================= */

/** LED port. */
#define LED_PORT                PORTC

/** LED pin bit mask. */
#define LED_PIN_bm              PIN6_bm

/* =========================================================================
 * Control-loop parameters
 * Simple proportional scaling: duty = (adc_raw * CTRL_SCALE_NUM) >> CTRL_SCALE_SHF
 * Maps 0–4095 ADC counts to 0–1023 PWM counts.
 * ========================================================================= */

/** Numerator shift for ADC → PWM mapping (1023/4095 ≈ 1/4 → right-shift 2). */
#define CTRL_ADC_TO_PWM_SHIFT   2U

/* =========================================================================
 * Threshold detection (AC)
 * ========================================================================= */

/** ADC threshold above which "high-power" state is entered. */
#define CTRL_HIGH_THRESHOLD     3072U   /* ~75 % of 4095 */

/** ADC threshold below which "low-power" state is re-entered (hysteresis). */
#define CTRL_LOW_THRESHOLD      1024U   /* ~25 % of 4095 */

#endif /* CONFIG_H_ */
