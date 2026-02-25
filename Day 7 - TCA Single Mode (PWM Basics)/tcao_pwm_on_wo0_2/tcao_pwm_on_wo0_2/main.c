#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define F_CLK_PER_HZ    (24000000UL)
#define PWM_TARGET_HZ   (10000UL)
#define PWM_DUTY_PC     (50U)

// Pick one prescaler and stick to it
#define TCA_DIV         (64UL)
#define TCA_CLKSEL_GC   (TCA_SINGLE_CLKSEL_DIV64_gc)

// PER = (F_CLK_PER / DIV / F_PWM) - 1
#define TCA_PER_VALUE   ((uint16_t)((F_CLK_PER_HZ / TCA_DIV / PWM_TARGET_HZ) - 1UL))
#define TCA_CMP_VALUE   ((uint16_t)(((uint32_t)(TCA_PER_VALUE + 1U) * PWM_DUTY_PC) / 100U))

/**
 * Initializes TCA0 in SINGLE mode, Single-Slope PWM, enabling WO0 output,
 * and the frequency and duty cycle defined. 
 */
static void tca0_pwm_wo0_init(void)
{
	// Route TCA0 outputs to PORTA (WO0 commonly on PA0 on this route) 
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

	// WO0 pin (PA0) as output 
	PORTA.DIRSET = PIN0_bm;

	// Stop timer
	TCA0.SINGLE.CTRLA = 0;

	// Single-slope PWM, enable WO0
	TCA0.SINGLE.CTRLB = (uint8_t)(TCA_SINGLE_WGMODE_SINGLESLOPE_gc |
	TCA_SINGLE_CMP0EN_bm);

	// Set period and duty 
	TCA0.SINGLE.PER  = TCA_PER_VALUE;
	TCA0.SINGLE.CMP0 = TCA_CMP_VALUE;

	// Start 
	TCA0.SINGLE.CTRLA = (uint8_t)(TCA_CLKSEL_GC | TCA_SINGLE_ENABLE_bm);
}

/**
 * Helper to sets duty in percent (0 to 100). 
 */
static void tca0_pwm_set_duty_percent(uint8_t duty_percent)
{
	uint32_t top = (uint32_t)TCA0.SINGLE.PER + 1UL;
	uint32_t cmp = (top * (uint32_t)duty_percent) / 100UL;

	if (cmp > 0)
	{
		cmp -= 1; /* keep within range for edge cases */
	}

	TCA0.SINGLE.CMP0 = (uint16_t)cmp;
}

/**
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

/**
 * Configures the main clock source to OSCHF and sets OSCHF to 24 MHz.
 * Disables the main prescaler.
 */
static void clock_to_24mhz_no_prescale(void)
{
	// OSCHF = 24 MHz 
	ccp_write_io(&CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc);

	// Main clock source = OSCHF
	ccp_write_io(&CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSCHF_gc);

	// Disable prescaler so CLK_PER = 24 MHz
	ccp_write_io(&CLKCTRL.MCLKCTRLB, 0x00);
}

/**
 * Application entry point. 
 */
int main(void)
{
	// Configures the main clock
	clock_to_24mhz_no_prescale();
	
	// Initializes TCA0
	tca0_pwm_wo0_init();

	while (1) {		
		// Ramp duty cycle up and down for visibility 
		for (uint8_t d = 0; d <= 100; d += 5)
		{
			tca0_pwm_set_duty_percent(d);
			_delay_ms(50);
		}

		for (int8_t d = 100; d >= 0; d -= 5)
		{
			tca0_pwm_set_duty_percent((uint8_t)d);
			_delay_ms(50);
		}		
	}
}