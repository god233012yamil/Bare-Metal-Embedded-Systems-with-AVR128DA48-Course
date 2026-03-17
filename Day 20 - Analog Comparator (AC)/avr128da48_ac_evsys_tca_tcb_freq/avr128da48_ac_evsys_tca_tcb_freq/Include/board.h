/**
 * @file board.h
 * @brief Board configuration for the AVR128DA48 AC + EVSYS + TCB capture lab.
 *
 * You MUST physically wire the WO0 pin to the selected AC0 positive input pin.
 */

#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>
#include <stdint.h>

/* CPU clock (adjust if your clock differs) */
#ifndef F_CPU
#define F_CPU (4000000UL)
#endif

/* PWM target frequency (Hz) */
#define PWM_TARGET_HZ      (1000UL)

/* ----------------------------- */
/* TCA0 WO0 routing              */
/* ----------------------------- */
/* Route TCA0 outputs using PORTMUX (update if your board uses a different route). */
#ifndef PORTMUX_TCA0_PORTA_gc
#define PORTMUX_TCA0_PORTA_gc (0u)
#endif

#define TCA0_ROUTE_GC   (PORTMUX_TCA0_PORTA_gc)

/* ----------------------------- */
/* Analog Comparator input       */
/* ----------------------------- */
/* Select AC0 positive input (must match a physical pin you can wire to WO0). */
#ifndef AC_MUXPOS_PIN0_gc
#define AC_MUXPOS_PIN0_gc (0u)
#endif

#define AC0_POS_MUX_GC   (AC_MUXPOS_PIN0_gc)

/* Negative input uses DAC reference. */
#ifndef AC_MUXNEG_DACREF_gc
#define AC_MUXNEG_DACREF_gc (0u)
#endif

#define AC0_NEG_MUX_GC   (AC_MUXNEG_DACREF_gc)

/* ----------------------------- */
/* LED (optional heartbeat)      */
/* ----------------------------- */
#define LED_PORT     PORTA
#define LED_PIN_bm   PIN2_bm

/**
 * @brief Initialize the LED GPIO.
 */
static inline void board_led_init(void)
{
    /* Set LED as output */
    LED_PORT.DIRSET = LED_PIN_bm;
    /* LED off */
    LED_PORT.OUTCLR = LED_PIN_bm;
}

/**
 * @brief Toggle the LED GPIO.
 */
static inline void board_led_toggle(void)
{
    /* Toggle LED */
    LED_PORT.OUTTGL = LED_PIN_bm;
}

#endif /* BOARD_H */
