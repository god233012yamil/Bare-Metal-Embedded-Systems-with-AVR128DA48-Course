/**
 * @file main.c
 * @brief AVR128DA48 PWM Frequency Measurement using TCA0, AC0, EVSYS, and TCB0.
 *
 * This project demonstrates PWM generation and frequency measurement on the
 * AVR128DA48 Curiosity Nano board using the following peripherals:
 *
 *  - TCA0: Generates a 50% duty cycle PWM signal on WO0 (PA0) at a known frequency.
 *  - AC0:  Detects the rising edge of the PWM signal fed into its positive input (PA7).
 *          Acts as an event generator via the Event System.
 *  - EVSYS: Routes AC0 output events to TCB0 as the event user.
 *  - TCB0: Configured in Frequency Measurement (FRQMEAS) mode. On each event
 *          (rising edge detected by AC0), TCB0 captures its counter value, which
 *          is used to compute and store the PWM frequency in a global variable.
 *
 * Hardware connections required on the AVR128DA48 Curiosity Nano:
 *  - Connect PA0 (TCA0 WO0 output) to PA7 (AC0 positive input, AIN0).
 *
 * @note Target device  : AVR128DA48
 * @note IDE            : Atmel Studio 7 / Microchip Studio
 * @note Device Pack    : AVR-Dx Device Pack 2.4.286
 * @note Toolchain      : AVR-GCC (GCC C Executable Project)
 * @note F_CPU          : 4 MHz (default internal oscillator after reset)
 *
 * @author  Your Name
 * @date    2025
 */

#define F_CPU 4000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Configuration constants
 * -------------------------------------------------------------------------- */

/**
 * @brief TCA0 prescaler selection.
 * Using DIV16 so the counter runs at F_CPU/16 = 250 000 Hz.
 */
#define TCA0_PRESCALER_DIV      TCA_SINGLE_CLKSEL_DIV16_gc

/**
 * @brief TCA0 period register value (PER).
 *
 * Desired PWM frequency = 1 kHz.
 * Timer clock = F_CPU / 16 = 250 000 Hz.
 * PER = (timer_clock / pwm_freq) - 1 = (250000 / 1000) - 1 = 249.
 */
#define TCA0_PER_VALUE          249U

/**
 * @brief TCA0 compare register value for 50 % duty cycle.
 * CMP0 = (PER + 1) / 2 = 125.
 */
#define TCA0_CMP0_VALUE         125U

/**
 * @brief TCB0 prescaler selection.
 * Using DIV2 so the capture counter runs at F_CPU/2 = 2 000 000 Hz.
 */
#define TCB0_PRESCALER_DIV      TCB_CLKSEL_DIV2_gc

/**
 * @brief Clock frequency seen by TCB0 after prescaling (Hz).
 * Used to convert captured ticks to frequency.
 */
#define TCB0_CLOCK_HZ           (F_CPU / 2UL)

/* --------------------------------------------------------------------------
 * Global variables
 * -------------------------------------------------------------------------- */

/**
 * @brief Most recently measured PWM frequency in Hz.
 *
 * Updated inside the TCB0 capture ISR every time a rising edge is detected
 * by AC0 and routed through EVSYS to TCB0. A value of 0 means no valid
 * measurement has been taken yet.
 */
volatile uint32_t g_pwm_frequency_hz = 0;

/* --------------------------------------------------------------------------
 * Function prototypes
 * -------------------------------------------------------------------------- */
static void clock_init(void);
static void tca0_pwm_init(void);
static void gpio_init(void);
static void ac0_init(void);
static void evsys_init(void);
static void tcb0_freq_measure_init(void);

/* ==========================================================================
 * Peripheral initialisation functions
 * ========================================================================== */

/**
 * @brief Initialise the main clock to use the internal 4 MHz oscillator.
 *
 * The AVR128DA48 starts up with the internal 4 MHz oscillator selected by
 * default, so no register writes are strictly required. This function is
 * provided for clarity and to make future clock changes straightforward.
 */
static void clock_init(void)
{
    /* Default: internal 4 MHz oscillator, no prescaler. Nothing to change. */
}

/**
 * @brief Configure GPIO pins used by TCA0, AC0, and the debug LED.
 *
 * Pin assignments:
 *  - PA0 : TCA0 WO0 output (PWM). Set to output; peripheral override handled
 *          by PORTMUX and TCA0 enable.
 *  - PA7 : AC0 positive input AIN0. Set to input, disable digital input buffer
 *          and pull-up to reduce noise on the analogue input.
 *  - PC6 : On-board LED (active low on Curiosity Nano). Set to output, starts
 *          high (LED off).
 */
static void gpio_init(void)
{
    /* PA0 – TCA0 WO0 PWM output */
    PORTA.DIRSET = PIN0_bm;

    /* PA7 – AC0 AIN0 positive input (analogue). Disable digital input buffer. */
    PORTA.DIRCLR  = PIN7_bm;
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;

    /* PC6 – on-board LED, output, default high (off) */
    PORTC.DIRSET  = PIN6_bm;
    PORTC.OUTSET  = PIN6_bm;
}

/**
 * @brief Initialise TCA0 in single-slope PWM mode to produce a 1 kHz,
 *        50 % duty cycle signal on WO0 (PA0).
 *
 * Timer clock  = F_CPU / DIV16 = 4 000 000 / 16 = 250 000 Hz
 * PWM period   = (PER + 1) ticks = 250 ticks ? 1 kHz
 * Duty cycle   = CMP0 / (PER + 1) = 125 / 250 = 50 %
 *
 * PORTMUX is configured to keep WO0 on PA0 (default alternate is not needed).
 */
static void tca0_pwm_init(void)
{
    /* Route WO0 to PA0 (default mapping – PORTMUX reset value is 0) */
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

    /* Set period and compare registers */
    TCA0.SINGLE.PER  = TCA0_PER_VALUE;
    TCA0.SINGLE.CMP0 = TCA0_CMP0_VALUE;

    /* Single-slope PWM, enable WO0 compare output, DIV16 prescaler, enable */
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc
                      | TCA_SINGLE_CMP0EN_bm;

    TCA0.SINGLE.CTRLA = TCA0_PRESCALER_DIV
                      | TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Initialise AC0 to compare AIN0 (PA7) against the internal DAC
 *        voltage reference (set to VDD/2) and generate an event on its output.
 *
 * The PWM signal driven onto PA0 is wired externally to PA7 (AIN0).
 * AC0 is configured to:
 *  - Use AIN0 (PA7) as the positive input.
 *  - Use the internal DAC output as the negative input (set to ~VDD/2 ? 1.65 V
 *    at 3.3 V supply so that the AC output mirrors the digital PWM level).
 *  - Enable the AC output to the Event System so that EVSYS can route it.
 *
 * @note The AC output event is a level signal. TCB0 in FRQMEAS mode uses the
 *       rising edge of the event channel to trigger a capture, which corresponds
 *       to the rising edge of the PWM signal.
 */
static void ac0_init(void)
{
    /* Set DAC reference: use VDD as the voltage reference */
    VREF.ACREF = VREF_REFSEL_VDD_gc;

    /* DAC0: enable, output to AC0 negative input, value = 128 (? VDD/2) */
    DAC0.CTRLA  = DAC_OUTEN_bm | DAC_ENABLE_bm;
    DAC0.DATA   = 128U << DAC_DATA_gp; /* 8-bit left-aligned value */

    /* AC0 MUX: positive = AIN0 (PA7), negative = DAC */
    AC0.MUXCTRLA = AC_MUXPOS_AINP0_gc | AC_MUXNEG_DACREF_gc;

    /* Enable AC0, output to pin disabled, enable event output (OUTEN not
     * needed for event routing – the AC output is automatically available
     * as an event generator once the AC is enabled). */
    AC0.CTRLA = AC_ENABLE_bm;
}

/**
 * @brief Configure EVSYS to route AC0 output to TCB0 event input.
 *
 * Channel 0 is used:
 *  - Generator : AC0 output (EVSYS_CHANNEL0_AC0_OUT_gc)
 *  - User      : TCB0 capture event input (EVSYS_USER_TCB0CAPT_CHANNEL0_gc)
 */
static void evsys_init(void)
{
    /* Assign AC0 output as generator on Event Channel 0 */
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_AC0_OUT_gc;

    /* Connect TCB0 capture input to Event Channel 0 */
    EVSYS.USERTCB0CAPT = EVSYS_USER_CHANNEL0_gc;
}

/**
 * @brief Initialise TCB0 in Frequency Measurement mode to capture the period
 *        of rising edges delivered by AC0 through EVSYS.
 *
 * In FRQMEAS mode the counter runs freely and resets on each capture event
 * (rising edge of the event signal). The captured value equals the number of
 * TCB clock ticks between two consecutive rising edges, i.e. the PWM period.
 *
 * TCB clock = F_CPU / 2 = 2 000 000 Hz (DIV2 prescaler).
 * Frequency  = TCB_CLOCK_HZ / captured_ticks.
 *
 * The capture interrupt is enabled so that the ISR can read CCMP and update
 * the global frequency variable.
 */
static void tcb0_freq_measure_init(void)
{
    /* Select clock source: CLK_PER divided by 2 */
    TCB0.CTRLA = TCB0_PRESCALER_DIV;

    /* Frequency measurement mode, enable capture event input */
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc;

    /* Enable capture interrupt */
    TCB0.INTCTRL = TCB_CAPT_bm;

    /* Enable TCB0 */
    TCB0.CTRLA |= TCB_ENABLE_bm;
}

/* ==========================================================================
 * Interrupt Service Routines
 * ========================================================================== */

/**
 * @brief TCB0 Capture ISR – computes PWM frequency from the captured period.
 *
 * In FRQMEAS mode, reading CCMP automatically clears the interrupt flag and
 * resets the counter for the next measurement. The captured value is the
 * number of TCB0 clock ticks that elapsed between the two most recent rising
 * edges of the AC0 output event.
 *
 * Frequency (Hz) = TCB0_CLOCK_HZ / captured_ticks
 *
 * The result is stored in the global variable @ref g_pwm_frequency_hz.
 * If the captured value is zero (should not occur in normal operation) the
 * frequency is left unchanged to avoid a division-by-zero fault.
 */
ISR(TCB0_INT_vect)
{
    /* Reading CCMP clears the interrupt flag (hardware behaviour in FRQMEAS). */
    uint16_t captured = TCB0.CCMP;

    if (captured != 0U)
    {
        g_pwm_frequency_hz = (uint32_t)TCB0_CLOCK_HZ / (uint32_t)captured;
    }
}

/* ==========================================================================
 * Main entry point
 * ========================================================================== */

/**
 * @brief Application entry point.
 *
 * Initialise all peripherals in the required order and then enters an
 * infinite super-loop. The frequency measurement is performed entirely in the
 * TCB0 capture ISR; the super-loop can be extended to use @ref g_pwm_frequency_hz
 * for display, communication, or control purposes.
 *
 * Initialisation order:
 *  1. Clock  – confirm 4 MHz internal oscillator.
 *  2. GPIO   – configure pins before enabling peripherals.
 *  3. TCA0   – start PWM generation on PA0.
 *  4. AC0    – enable analogue comparator (after GPIO so AIN0 buffer is off).
 *  5. EVSYS  – connect AC0 ? TCB0 before enabling TCB0.
 *  6. TCB0   – start frequency measurement, enable capture interrupt.
 *  7. sei()  – enable global interrupts.
 *
 * @return This function never returns (embedded super-loop).
 */
int main(void)
{
    // Initialise the main clock
	clock_init();
    
	// Configure GPIO pins used
	gpio_init();
    
	// Initialise TCA0 to generate a PWM signal
	tca0_pwm_init();
    
	// Initialise AC0 to compare
	ac0_init();
    
	// Configure EVSYS to route AC0 output to TCB0 event input
	evsys_init();
    
	// Initialise TCB0 in Frequency Measurement mode
	tcb0_freq_measure_init();
	
	// Enable global interrupts 
    sei(); 

    while (1)
    {
        /*
         * g_pwm_frequency_hz is updated by the TCB0 ISR.
         * Add application logic here (e.g. USART output, LED indication).
         *
         * Example: toggle LED if measured frequency is within ±5 % of 1 kHz.
         *   if (g_pwm_frequency_hz >= 950 && g_pwm_frequency_hz <= 1050)
         *       PORTC.OUTTGL = PIN6_bm;
         */
    }

    return 0; /* Never reached */
}
