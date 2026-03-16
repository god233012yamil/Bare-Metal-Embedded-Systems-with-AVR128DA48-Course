/**
 * @file main.c
 * @brief DAC + Analog Comparator demo (AVR128DA48, register-level).
 *
 * Demo goal:
 * - Use DAC0 to generate an adjustable analog threshold voltage.
 * - Feed DAC0 output (via a jumper wire) into AC0 negative input.
 * - Feed an external analog signal into AC0 positive input.
 * - Trigger interrupt when signal crosses the threshold, and toggle an LED.
 *
 * Hardware wiring:
 * 1) Connect DAC0 OUT pin -> AC0 negative input pin (AINN).
 * 2) Connect your analog signal source -> AC0 positive input pin (AINP).
 * 3) Common GND (already common on the board).
 *
 * Notes:
 * - This example uses the internal 4.096V reference for the DAC.
 * - Comparator inputs must be on valid analog-capable pins for AC0.
 * - You must adjust AC0.MUXCTRLA MUXPOS/MUXNEG selections to match your pins.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define VREF_MV          4096u
#define DAC_MAX_CODE     1023u

/* Pick an LED pin you can observe easily (adjust for your board). */
#define LED_PORT         PORTA
#define LED_PIN_bm       PIN7_bm

/* Simple delay for visible LED toggling and DAC stepping (crude). */
static void delay_cycles(volatile uint32_t n)
{
    while (n--)
    {
        __asm__ __volatile__("nop");
    }
}

/**
 * @brief Initialize LED GPIO as output.
 */
static void LED_init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;
    LED_PORT.OUTCLR = LED_PIN_bm;
}

/**
 * @brief Toggle LED.
 */
static void LED_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/**
 * @brief Initialize VREF and DAC0 for threshold generation.
 *
 * Uses internal 4.096V reference and enables DAC0 output buffer.
 */
static void DAC0_init(void)
{
    /* Select 4.096V reference for DAC0 */
    VREF.DAC0REF = VREF_REFSEL_4V096_gc;

    /* Enable DAC output buffer and DAC */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

/**
 * @brief Convert millivolts to a 10-bit DAC code (0..1023), based on VREF_MV.
 */
static uint16_t dac_code_from_mv(uint16_t mv)
{
    uint32_t code;

    if (mv >= VREF_MV)
    {
        return DAC_MAX_CODE;
    }

    code = ((uint32_t)mv * (uint32_t)DAC_MAX_CODE) / (uint32_t)VREF_MV;
    return (uint16_t)code;
}

/**
 * @brief Set DAC0 threshold in millivolts.
 */
static void DAC0_set_threshold_mv(uint16_t mv)
{
    uint16_t code = dac_code_from_mv(mv);
    DAC0.DATA = code;
}

/**
 * @brief Initialize AC0 to compare AINP (signal) against AINN (threshold from DAC).
 *
 * You MUST select the correct MUXPOS/MUXNEG based on your wiring.
 *
 * Example intention:
 * - MUXPOS: AC positive input pin (external signal)
 * - MUXNEG: AC negative input pin (wired from DAC0 OUT)
 */
static void AC0_init(void)
{
    /*
     * Disable AC0 before configuration (good practice).
     * Some AVR devices require disabling before changing mux settings.
     */
    AC0.CTRLA = 0;

    /*
     * Select comparator inputs.
     * Adjust these two fields to match your actual pins:
     *
     * - AC_MUXPOS_AINx_gc for the external signal pin
     * - AC_MUXNEG_AINy_gc for the DAC threshold pin
     *
     * Common patterns:
     * - MUXPOS = AINP pin
     * - MUXNEG = AINN pin
     */
    AC0.MUXCTRLA =
        AC_MUXPOS_AIN0_gc |   /* TODO: change AIN0 to your signal input */
        AC_MUXNEG_AIN1_gc;    /* TODO: change AIN1 to your DAC threshold input */

    /*
     * Optional: enable hysteresis to reduce chatter near threshold.
     * Hysteresis levels depend on the device. If available, pick a small amount.
     */
    AC0.CTRLA |= AC_HYSMODE_SMALL_gc;

    /*
     * Interrupt configuration:
     * Trigger on output toggle or rising edge.
     *
     * - If you want "signal crossed above threshold": use RISING.
     * - If you want both directions: use BOTHEDGES (if supported).
     */
    AC0.INTCTRL = AC_CMP_bm;               /* Enable comparator interrupt */
    AC0.CTRLB   = AC_INTMODE_RISING_gc;    /* Interrupt on rising output */

    /* Clear any stale flags */
    AC0.STATUS = AC_CMP_bm;

    /* Enable comparator */
    AC0.CTRLA |= AC_ENABLE_bm;
}

/**
 * @brief AC0 interrupt: fires when comparator output crosses configured edge.
 *
 * For rising mode:
 * - This typically means the signal exceeded the threshold.
 */
ISR(AC0_AC_vect)
{
    /* Clear interrupt flag */
    AC0.STATUS = AC_CMP_bm;

    /* Visual indication */
    LED_toggle();
}

int main(void)
{
    LED_init();
    DAC0_init();

    /*
     * Start with a mid-scale threshold: ~2.048V.
     * Wire DAC OUT -> AC0 negative input pin.
     */
    DAC0_set_threshold_mv(2048);

    AC0_init();

    /* Global interrupts on */
    sei();

    /*
     * Demo behavior:
     * - Slowly sweep the DAC threshold up and down.
     * - Keep your external signal steady (pot, sensor, DC level).
     * - Each time the threshold sweep crosses the signal level,
     *   the comparator output will transition and toggle the LED.
     *
     * This makes the DAC+AC interaction very obvious on real hardware.
     */
    while (1)
    {
        uint16_t mv;

        /* Sweep up: 0.5V to 3.5V */
        for (mv = 500; mv <= 3500; mv += 50)
        {
            DAC0_set_threshold_mv(mv);
            delay_cycles(150000);
        }

        /* Sweep down: 3.5V to 0.5V */
        for (mv = 3500; mv >= 500; mv -= 50)
        {
            DAC0_set_threshold_mv(mv);
            delay_cycles(150000);

            if (mv < 550) /* prevent underflow on uint16_t */
            {
                break;
            }
        }
    }
}