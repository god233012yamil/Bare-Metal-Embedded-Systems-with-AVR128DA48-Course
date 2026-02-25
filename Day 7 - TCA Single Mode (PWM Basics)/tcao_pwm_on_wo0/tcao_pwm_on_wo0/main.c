/*
 * AVR128DA48 - TCA0 PWM on WO0 (Single-Slope PWM)
 *
 * Bare-metal example (no MCC/START/ASF).
 *
 * What this provides:
 * - Initializes the main clock to OSCHF @ 24 MHz with prescaler /2 (CLK_PER = 12 MHz)
 * - Configures TCA0 in SINGLE mode, Single-Slope PWM, output on WO0 (PA0)
 * - Provides a function to set PWM frequency (Hz) and duty cycle (%) at runtime
 *
 * Notes:
 * - WO0 pin routing depends on PORTMUX.TCAROUTEA. The default routing varies by device/package.
 * - This example includes a default route selection. If you do not see the waveform on your
 *   expected pin, check the AVR128DA48 datasheet PORTMUX section and change TCAROUTEA setting.
 */

#include <avr/io.h>
#include <stdint.h>

/* -------------------------- Clock assumptions -------------------------- */
/*
 * This example configures:
 * - OSCHF = 24 MHz
 * - Main prescaler = /2
 * => CLK_PER = 12 MHz
 *
 * If you change clock_init_24mhz_presc2(), update CLK_PER_HZ accordingly.
 */
#define CLK_PER_HZ (12000000UL)

/* -------------------------- PWM output pin ----------------------------- */
/*
 * WO0 pin depends on routing. You MUST verify the actual pin on your board/package.
 *
 * Common practice on AVR Dx:
 * - PORTMUX.TCAROUTEA selects where TCA0 WO0..WO5 appear.
 *
 * For safety, we:
 * - Set routing to DEFAULT
 * - Configure a candidate pin as output (PORTA PIN0 shown as an example)
 *
 * If your WO0 is not on PA0, change these macros to match your board.
 */
#define PWM_WO0_PORT   PORTA
#define PWM_WO0_PIN_bm PIN0_bm

/* -------------------------- CCP helper --------------------------------- */
/**
 * ccp_write_io
 *
 * Writes to a CCP-protected I/O register.
 *
 * Args:
 *   addr: Pointer to CCP-protected register.
 *   value: Value to write.
 */
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;
    *addr = value;
}

/* -------------------------- Clock init --------------------------------- */
/**
 * clock_init_24mhz_presc2
 *
 * Configures the main clock source to OSCHF and sets OSCHF to 24 MHz.
 * Enables the main prescaler /2 so CLK_PER = 12 MHz.
 *
 * This matches the clock style you have been using in your course.
 */
static void clock_init_24mhz_presc2(void)
{
    /* Select main clock source: OSCHF, CLKOUT disabled */
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLA,
                 (uint8_t)(CLKCTRL_CLKSEL_OSCHF_gc | (0 << CLKCTRL_CLKOUT_bp)));

    /* Set OSCHF frequency: 24 MHz (no autotune, no standby run) */
    ccp_write_io((volatile uint8_t *)&CLKCTRL.OSCHFCTRLA,
                 (uint8_t)(CLKCTRL_FRQSEL_24M_gc |
                           (0 << CLKCTRL_AUTOTUNE_bp) |
                           (0 << CLKCTRL_RUNSTDBY_bp)));

    /* Enable prescaler, divide by 2 -> CLK_PER = 12 MHz */
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLB,
                 (uint8_t)(CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc));
}

/* -------------------------- PWM core ----------------------------------- */

/**
 * tca0_pwm_route_wo0_default
 *
 * Routes TCA0 outputs using the DEFAULT PORTMUX setting.
 *
 * If your WO0 output is not on the expected pin, change TCAROUTEA to the
 * alternate route required by your board/package.
 */
static void tca0_pwm_route_wo0_default(void)
{
    /* Route TCA0 outputs (WO0..WO5) to default pins */
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc; /* If this symbol differs in your pack, use the DEFAULT/PORTA route enum. */
}

/**
 * tca0_pwm_init
 *
 * Initializes TCA0 in SINGLE mode, Single-Slope PWM, enabling WO0 output.
 *
 * This does not set frequency/duty yet. Call tca0_pwm_set() after init.
 */
static void tca0_pwm_init(void)
{
    /* Route WO0 to pins (board-dependent) */
    tca0_pwm_route_wo0_default();

    /* Configure WO0 pin as output (adjust pin to match your board) */
    PWM_WO0_PORT.DIRSET = PWM_WO0_PIN_bm;

    /* Stop timer before configuration */
    TCA0.SINGLE.CTRLA = 0;

    /* Single-slope PWM, enable WO0 compare channel */
    TCA0.SINGLE.CTRLB = (uint8_t)(TCA_SINGLE_WGMODE_SINGLESLOPE_gc |
                                  TCA_SINGLE_CMP0EN_bm);

    /* Start from a known state */
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.PER = 0xFFFF;
    TCA0.SINGLE.CMP0 = 0;

    /* Clear flags */
    TCA0.SINGLE.INTFLAGS = 0xFF;
}

/**
 * tca0_pwm_pick_prescaler
 *
 * Picks the smallest prescaler that allows PER to fit in 16 bits for the given frequency.
 *
 * Args:
 *   freq_hz: Desired PWM frequency in Hz.
 *   top_out: Output PER value (TOP).
 *   clk_sel_out: Output prescaler selection bits for TCA0.SINGLE.CTRLA.
 *
 * Returns:
 *   1 if success, 0 if the frequency is not achievable with 16-bit PER.
 */
static uint8_t tca0_pwm_pick_prescaler(uint32_t freq_hz, uint16_t *top_out, uint8_t *clk_sel_out)
{
    /* Available prescalers for TCA SINGLE clock selection */
    static const struct {
        uint16_t div;
        uint8_t clk_sel_gc;
    } opts[] = {
        { 1,    TCA_SINGLE_CLKSEL_DIV1_gc    },
        { 2,    TCA_SINGLE_CLKSEL_DIV2_gc    },
        { 4,    TCA_SINGLE_CLKSEL_DIV4_gc    },
        { 8,    TCA_SINGLE_CLKSEL_DIV8_gc    },
        { 16,   TCA_SINGLE_CLKSEL_DIV16_gc   },
        { 64,   TCA_SINGLE_CLKSEL_DIV64_gc   },
        { 256,  TCA_SINGLE_CLKSEL_DIV256_gc  },
        { 1024, TCA_SINGLE_CLKSEL_DIV1024_gc }
    };

    if (freq_hz == 0)
        return 0;

    for (uint8_t i = 0; i < (uint8_t)(sizeof(opts) / sizeof(opts[0])); i++)
    {
        uint32_t tca_clk = CLK_PER_HZ / opts[i].div;
        uint32_t top = (tca_clk / freq_hz);

        if (top == 0)
            continue;

        top -= 1;

        if (top <= 0xFFFFUL)
        {
            *top_out = (uint16_t)top;
            *clk_sel_out = opts[i].clk_sel_gc;
            return 1;
        }
    }

    return 0;
}

/**
 * tca0_pwm_set
 *
 * Sets TCA0 PWM frequency and duty cycle for WO0.
 *
 * Args:
 *   freq_hz: PWM frequency in Hz.
 *   duty_percent: Duty cycle in percent (0..100).
 *
 * Behavior:
 * - Chooses a prescaler automatically to fit PER into 16 bits.
 * - Updates PER and CMP0 accordingly.
 * - Starts the timer if configuration is valid.
 */
static void tca0_pwm_set(uint32_t freq_hz, uint8_t duty_percent)
{
    uint16_t top = 0;
    uint8_t clk_sel = 0;

    if (duty_percent > 100)
        duty_percent = 100;

    if (!tca0_pwm_pick_prescaler(freq_hz, &top, &clk_sel))
    {
        /* Not achievable: stop timer and force output low duty */
        TCA0.SINGLE.CTRLA = 0;
        TCA0.SINGLE.PER = 0xFFFF;
        TCA0.SINGLE.CMP0 = 0;
        return;
    }

    /*
     * Single-slope PWM:
     * - PER sets period (TOP)
     * - CMP0 sets compare threshold
     *
     * Duty mapping:
     * - duty=0%   -> CMP0=0 (effectively always low)
     * - duty=100% -> CMP0=TOP+1 (effectively always high on many AVRs)
     *
     * To avoid edge-case weirdness, clamp 100% to TOP+1 if supported by silicon,
     * otherwise use TOP. Here we implement TOP+1 using 32-bit math then clamp.
     */
    uint32_t period_counts = (uint32_t)top + 1UL;
    uint32_t cmp = (period_counts * (uint32_t)duty_percent) / 100UL;

    if (cmp > 0xFFFFUL)
        cmp = 0xFFFFUL;

    /* Stop timer while updating critical registers */
    uint8_t old_ctrla = TCA0.SINGLE.CTRLA;
    TCA0.SINGLE.CTRLA = 0;

    TCA0.SINGLE.PER  = top;
    TCA0.SINGLE.CMP0 = (uint16_t)cmp;

    /* Restart timer with selected prescaler */
    TCA0.SINGLE.CTRLA = (uint8_t)(clk_sel | TCA_SINGLE_ENABLE_bm);

    (void)old_ctrla; /* kept to make it easy to extend later */
}

/* -------------------------- Demo main ---------------------------------- */
/**
 * main
 *
 * Demonstrates runtime PWM configuration:
 * - Initializes clock and TCA0 PWM
 * - Sweeps duty cycle at a fixed frequency
 *
 * Note:
 * - This demo uses a crude software delay loop. Replace it with your 1ms tick later.
 */
int main(void)
{
    // Configure system clock
	clock_init_24mhz_presc2();
	
	// Initializes TCA0
    tca0_pwm_init();

    // Sets TCA0 PWM frequency to 5KHz and the duty cycle to 10% 
    tca0_pwm_set(20000UL, 50);

    while (1)
    {
        // Sweep duty 10% -> 90%
        for (uint8_t d = 10; d <= 90; d += 10) {
            tca0_pwm_set(5000UL, d);

            // crude delay
            for (volatile uint32_t i = 0; i < 200000UL; i++) {
                __asm__ __volatile__("nop");
            }
        }

        // Change frequency example: 1 kHz at 50%
        tca0_pwm_set(1000UL, 50);

		// crude delay
        for (volatile uint32_t i = 0; i < 400000UL; i++) {
            __asm__ __volatile__("nop");
        }

    }
}