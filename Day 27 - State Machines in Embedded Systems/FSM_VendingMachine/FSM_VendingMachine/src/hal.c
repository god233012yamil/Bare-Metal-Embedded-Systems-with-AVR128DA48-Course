/**
 * @file    hal.c
 * @brief   Hardware Abstraction Layer implementation for AVR128DA48 Curiosity Nano.
 *
 * All register-level code lives here.  The rest of the project uses only the
 * functions declared in hal.h.
 *
 * Clock
 * -----
 *  Default after reset: internal OSCHF at 4 MHz.  No CLKCTRL changes needed.
 *  F_CPU must be kept consistent with the actual clock in this file.
 *
 * UART baud rate calculation (USART0, asynchronous normal mode)
 * ----------------------------------------------------------------
 *  BAUD register = (64 * F_CPU) / (16 * baud_rate)
 *                = (4 * F_CPU) / baud_rate
 *  For 9600 baud @ 4 MHz: BAUD = (4 * 4000000) / 9600 = 1667
 *
 * TCB0 period for 1 ms tick
 * --------------------------
 *  CLK_PER = 4 MHz, no prescaler.
 *  Counts for 1 ms: 4000000 / 1000 = 4000  => CCMP = 3999 (0-based)
 *
 * ADC0
 * ----
 *  VREF = VDD (~3.3 V), 12-bit, prescaler DIV16 → 250 kHz ADC clock.
 *  Input:  PD3 / AIN3.
 *
 * Author  : FSM Demo Project
 * Target  : AVR128DA48 Curiosity Nano
 * Toolchain: Atmel Studio 7 / avr-gcc
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#include "hal.h"

/* =========================================================================
 * Constants and pin definitions
 * ========================================================================= */

#define F_CPU_HZ        4000000UL   /**< System clock (OSCHF default 4 MHz) */

/* --- GPIO ---------------------------------------------------------------- */
#define LED_PIN         PIN6_bm     /**< PC6 – active-LOW onboard LED        */
#define BTN_SW0_PIN     PIN7_bm     /**< PC7 – active-LOW onboard SW0        */
#define BTN_COIN_PIN    PIN2_bm     /**< PA2 – active-LOW coin button        */
#define BTN_DISP_PIN    PIN3_bm     /**< PA3 – active-LOW dispense button    */
#define UART_TX_PIN     PIN0_bm     /**< PA0 – USART0 TXD (default mux)     */
#define UART_RX_PIN     PIN1_bm     /**< PA1 – USART0 RXD (default mux)     */
#define ADC_PIN         PIN3_bm     /**< PD3 – AIN3, analog input            */

/* --- UART baud ----------------------------------------------------------- */
/*  BAUD = (4 * F_CPU) / desired_baud                                        */
#define USART_BAUD_REG  ((uint16_t)((4UL * F_CPU_HZ) / 9600UL))

/* --- TCB0 tick period ---------------------------------------------------- */
/*  CCMP = (F_CPU / 1000) - 1   for a 1 ms periodic interrupt               */
#define TCB0_CCMP_1MS   ((uint16_t)(F_CPU_HZ / 1000UL - 1UL))

/* --- Software debounce --------------------------------------------------- */
#define DEBOUNCE_MS     20u         /**< 20 ms debounce window               */

/* =========================================================================
 * Module-private state
 * ========================================================================= */

/** @brief System tick counter, incremented every 1 ms by TCB0 ISR. */
static volatile uint32_t g_tick_ms = 0;

/** @brief ADC result ready flag – set by ADC0 RESRDY ISR. */
static volatile bool     g_adc_ready  = false;

/** @brief Latest 12-bit ADC conversion result. */
static volatile uint16_t g_adc_result = 0;

/**
 * @brief Per-button debounce state.
 *
 * last_raw   – raw pin state sampled on the previous poll.
 * stable     – debounced (confirmed) state.
 * timestamp  – tick value when last_raw changed (for timeout).
 * edge_flag  – true when a press edge has been detected and not yet consumed.
 */
typedef struct
{
    bool     last_raw;   /**< Raw pin level on last poll  */
    bool     stable;     /**< Debounced (confirmed) level */
    uint32_t timestamp;  /**< Tick snapshot of last change */
    bool     edge_flag;  /**< Unconsumed press edge        */
} btn_state_t;

/** @brief Debounce state for each button. */
static btn_state_t g_btn[HAL_BTN_COUNT];

/* =========================================================================
 * ISR handlers
 * ========================================================================= */

/**
 * @brief TCB0 capture/compare match ISR – fires every 1 ms.
 *
 * Increments the global millisecond counter used by hal_tick_ms() and
 * hal_elapsed_ms().  The CAPT interrupt flag is cleared by writing 1 to it.
 */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;   /* clear the interrupt flag */
    g_tick_ms++;
}

/**
 * @brief ADC0 result-ready ISR – fires when a conversion completes.
 *
 * Reads the 12-bit result into the module variable and sets the ready flag.
 * The main application is notified via hal_adc_ready().
 */
ISR(ADC0_RESRDY_vect)
{
    ADC0.INTFLAGS  = ADC_RESRDY_bm; /* clear the interrupt flag */
    g_adc_result   = ADC0.RES;      /* latch the result         */
    g_adc_ready    = true;
}

/* =========================================================================
 * Private helpers
 * ========================================================================= */

/**
 * @brief Read the raw (non-debounced) logical state of a button.
 *
 * Returns true when the button is physically pressed (pin is LOW due to
 * active-LOW wiring with internal pull-up).
 *
 * @param[in] btn  Button identifier.
 * @return true if the button pin is currently driven LOW (pressed).
 */
static bool btn_raw_pressed(hal_btn_id_t btn)
{
    switch (btn)
    {
        case HAL_BTN_SW0:
            return !(PORTC.IN & BTN_SW0_PIN);

        case HAL_BTN_COIN:
            return !(PORTA.IN & BTN_COIN_PIN);

        case HAL_BTN_DISPENSE:
            return !(PORTA.IN & BTN_DISP_PIN);

        default:
            return false;
    }
}

/* =========================================================================
 * HAL API – Initialisation
 * ========================================================================= */

/**
 * @brief Initialise all hardware peripherals.
 *
 * Call once at startup before enabling global interrupts.
 *
 * Peripheral setup order:
 *   1. GPIO  – LED output, button inputs with pull-ups, UART pins.
 *   2. USART0 – 9600 8N1, TX only (RX pin reserved).
 *   3. ADC0  – 12-bit, VDD reference, AIN3 input.
 *   4. TCB0  – 1 ms periodic interrupt for system tick.
 *   5. Debounce state reset.
 */
void hal_init(void)
{
    /* ------------------------------------------------------------------
     * 1. GPIO
     * ------------------------------------------------------------------ */

    /* LED: output, start HIGH (LED off – active LOW) */
    PORTC.OUTSET  = LED_PIN;
    PORTC.DIRSET  = LED_PIN;

    /* SW0: input, internal pull-up, falling-edge sense (interrupt unused –
     * polled via hal_btn_pressed() instead to keep code simple)           */
    PORTC.DIRCLR   = BTN_SW0_PIN;
    PORTC.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;

    /* COIN button on PA2: input, pull-up */
    PORTA.DIRCLR   = BTN_COIN_PIN;
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;

    /* DISPENSE button on PA3: input, pull-up */
    PORTA.DIRCLR   = BTN_DISP_PIN;
    PORTA.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_INTDISABLE_gc;

    /* ADC input PD3: disable digital input buffer to reduce noise/current */
    PORTD.DIRCLR   = ADC_PIN;
    PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;

    /* USART0 TX (PA0): output; RX (PA1): input (default after reset) */
    PORTA.DIRSET  = UART_TX_PIN;
    PORTA.DIRCLR  = UART_RX_PIN;

    /* ------------------------------------------------------------------
     * 2. USART0 – 9600 baud, 8-N-1, asynchronous normal mode
     * ------------------------------------------------------------------ */
    USART0.BAUD  = USART_BAUD_REG;
    USART0.CTRLB = USART_TXEN_bm;  /* enable TX; leave RX disabled */

    /* ------------------------------------------------------------------
     * 3. ADC0 – 12-bit, VDD reference, AIN3, result-ready interrupt
     * ------------------------------------------------------------------ */
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;               /* VDD as reference   */
    ADC0.CTRLC   = ADC_PRESC_DIV16_gc;               /* 4MHz/16 = 250 kHz  */
    ADC0.MUXPOS  = ADC_MUXPOS_AIN3_gc;               /* PA3 / AIN3 input   */
    ADC0.INTCTRL = ADC_RESRDY_bm;                     /* RESRDY interrupt   */
    ADC0.CTRLA   = ADC_RESSEL_12BIT_gc | ADC_ENABLE_bm;

    /* ------------------------------------------------------------------
     * 4. TCB0 – periodic interrupt every 1 ms (system tick)
     *
     *    Mode  : Periodic Interrupt (TCB_CNTMODE_INT_gc)
     *    Clock : CLK_PER (4 MHz), no prescaler
     *    CCMP  : 3999  => interrupt period = (3999+1) / 4000000 = 1 ms
     * ------------------------------------------------------------------ */
    TCB0.CCMP    = TCB0_CCMP_1MS;
    TCB0.INTCTRL = TCB_CAPT_bm;                       /* CAPT/OVF interrupt */
    TCB0.CTRLB   = TCB_CNTMODE_INT_gc;               /* Periodic int mode  */
    TCB0.CTRLA   = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm; /* CLK_PER, run */

    /* ------------------------------------------------------------------
     * 5. Reset software debounce state for all buttons
     * ------------------------------------------------------------------ */
    for (uint8_t i = 0; i < HAL_BTN_COUNT; i++)
    {
        g_btn[i].last_raw  = false;
        g_btn[i].stable    = false;
        g_btn[i].timestamp = 0;
        g_btn[i].edge_flag = false;
    }
}

/* =========================================================================
 * HAL API – LED
 * ========================================================================= */

/**
 * @brief Turn LED0 on (drive PC6 LOW).
 */
void hal_led_on(void)
{
    PORTC.OUTCLR = LED_PIN;
}

/**
 * @brief Turn LED0 off (drive PC6 HIGH).
 */
void hal_led_off(void)
{
    PORTC.OUTSET = LED_PIN;
}

/**
 * @brief Toggle LED0.
 */
void hal_led_toggle(void)
{
    PORTC.OUTTGL = LED_PIN;
}

/* =========================================================================
 * HAL API – Buttons
 * ========================================================================= */

/**
 * @brief Poll a button and return true once per confirmed press.
 *
 * Debounce algorithm (software, polling-based):
 *   - Sample the raw pin level.
 *   - If it differs from the last sample, record a timestamp and update
 *     last_raw.
 *   - If it has been stable (unchanged) for >= DEBOUNCE_MS milliseconds,
 *     update the confirmed state.
 *   - When the confirmed state transitions from NOT-pressed to PRESSED,
 *     set edge_flag and return true (only once).
 *
 * @param[in] btn  Button to query.
 * @return true on the leading edge of a confirmed press, false otherwise.
 */
bool hal_btn_pressed(hal_btn_id_t btn)
{
    if (btn >= HAL_BTN_COUNT) return false;

    btn_state_t *b   = &g_btn[btn];
    bool         raw = btn_raw_pressed(btn);

    /* Detect raw-level change and restart debounce timer */
    if (raw != b->last_raw)
    {
        b->last_raw  = raw;
        b->timestamp = g_tick_ms;  /* snapshot current time */
    }

    /* Check whether the level has been stable long enough */
    if (hal_elapsed_ms(b->timestamp) >= DEBOUNCE_MS)
    {
        if (raw != b->stable)
        {
            b->stable = raw;

            /* Rising edge of a PRESS: set the one-shot flag */
            if (b->stable == true)
            {
                b->edge_flag = true;
            }
        }
    }

    /* Consume the edge flag and report to the caller */
    if (b->edge_flag)
    {
        b->edge_flag = false;
        return true;
    }

    return false;
}

/* =========================================================================
 * HAL API – ADC
 * ========================================================================= */

/**
 * @brief Start a single ADC conversion.
 *
 * Writes the STCONV bit.  The ISR will set g_adc_ready when done.
 */
void hal_adc_start(void)
{
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief Check whether an ADC result is available.
 *
 * @return true if the RESRDY ISR has stored a new sample.
 */
bool hal_adc_ready(void)
{
    return g_adc_ready;
}

/**
 * @brief Read and consume the latest ADC result.
 *
 * @return 12-bit ADC sample (0–4095).
 */
uint16_t hal_adc_read(void)
{
    g_adc_ready = false;     /* clear ready flag so caller knows it was read */
    return g_adc_result;
}

/* =========================================================================
 * HAL API – System tick
 * ========================================================================= */

/**
 * @brief Return elapsed milliseconds since power-on.
 *
 * Reads g_tick_ms with interrupts briefly disabled to ensure an atomic
 * 32-bit read on an 8-bit CPU.
 */
uint32_t hal_tick_ms(void)
{
    uint32_t t;
    uint8_t  sreg = SREG;   /* save interrupt state */
    cli();
    t = g_tick_ms;
    SREG = sreg;            /* restore interrupt state */
    return t;
}

/**
 * @brief Return milliseconds elapsed since @p start_ms.
 *
 * Uses subtraction modulo 2^32 so it handles the 32-bit wrap correctly
 * without any special-case code.
 *
 * @param[in] start_ms  Value previously obtained from hal_tick_ms().
 */
uint32_t hal_elapsed_ms(uint32_t start_ms)
{
    return hal_tick_ms() - start_ms;  /* wraps correctly in unsigned arithmetic */
}

/* =========================================================================
 * HAL API – UART
 * ========================================================================= */

/**
 * @brief Transmit one character over USART0, blocking until DREIF is set.
 *
 * @param[in] c  Byte to transmit.
 */
void hal_uart_putc(char c)
{
    /* Wait for the transmit data register to be empty */
    while (!(USART0.STATUS & USART_DREIF_bm))
    {
        /* busy-wait */
    }
    USART0.TXDATAL = (uint8_t)c;
}

/**
 * @brief Transmit a null-terminated string over USART0.
 *
 * @param[in] s  String to send.
 */
void hal_uart_puts(const char *s)
{
    while (*s)
    {
        hal_uart_putc(*s++);
    }
}

/**
 * @brief Transmit a 16-bit unsigned integer as a decimal ASCII string.
 *
 * Uses a small local buffer; no heap allocation.
 *
 * @param[in] value  Integer to print.
 */
void hal_uart_put_u16(uint16_t value)
{
    char    buf[6];    /* max 5 digits for 65535 + null terminator */
    uint8_t idx = sizeof(buf) - 1;

    buf[idx] = '\0';

    if (value == 0)
    {
        buf[--idx] = '0';
    }
    else
    {
        while (value > 0 && idx > 0)
        {
            buf[--idx] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    hal_uart_puts(&buf[idx]);
}
