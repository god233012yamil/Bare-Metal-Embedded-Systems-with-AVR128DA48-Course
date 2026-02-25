#include <avr/io.h>
#include <util/delay.h>

/**
 * Initializes TCA0 in Split mode. 
 */
static void tca0_split_pwm_init(void)
{
    // Stop TCA0 before configuration 
    TCA0.SINGLE.CTRLA = 0;

    // Set the pin positions for TCA0 signals (PA0, PA1, PA2, PA3, PA4, PA5)
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc; 

    // Configure output pins as outputs (PA0, PA1, PA2, PA3)
    PORTA.DIRSET = PIN0_bm | PIN1_bm | PIN3_bm | PIN4_bm;

    // Enable Split mode
    TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

    /*
     * Select a prescaler for the TCA clock.
     * This affects both L and H timers.
     */
    TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc;

    /*
     * Set independent periods for L and H.
     * LPER and HPER are 8-bit (0..255).
     */
    TCA0.SPLIT.LPER = 199;  // Low side PWM period 
    TCA0.SPLIT.HPER = 99;   // High side PWM period (different freq) 

    /*
     * Set compare values (duty control).
     * LCMPx -> affects WO0/WO1/WO2 (low side)
     * HCMPx -> affects WO3/WO4/WO5 (high side)
     */
    TCA0.SPLIT.LCMP0 = 50;  // WO0 duty 
    TCA0.SPLIT.LCMP1 = 150; // WO1 duty 

    TCA0.SPLIT.HCMP0 = 20;  // WO3 duty 
    TCA0.SPLIT.HCMP1 = 70;  // WO4 duty 

    /*
     * Enable waveform outputs.
     * In split mode, you enable the WO channels you need.
     */
    TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm
                     | TCA_SPLIT_LCMP1EN_bm
                     | TCA_SPLIT_HCMP0EN_bm
                     | TCA_SPLIT_HCMP1EN_bm;

    // Enable TCA0, and start the timer
    TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
}

/**
 * Helper to sets duty cycle in percent (0 to 100) for the 
 * Lower Compare Channel 0. 
 */
static void tca0_split_set_lcmp0_duty_percent(uint8_t duty_percent)
{
	uint16_t top = (uint16_t)TCA0.SPLIT.LPER + 1U;
	uint16_t cmp = (top * (uint16_t)duty_percent) / 100U;

	if (cmp > 0)
	{
		cmp -= 1;
	}

	TCA0.SPLIT.LCMP0 = (uint8_t)cmp;
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
    // Configures the main clock to 24MHz
    clock_to_24mhz_no_prescale();
	
	// Initializes TCA0 in Split mode. 
	tca0_split_pwm_init();

    while (1)
    {
        for (uint8_t d = 0; d <= 100; d += 5)
        {
            tca0_split_set_lcmp0_duty_percent(d);
            _delay_ms(50); // crude delay
        }

        for (int8_t d = 100; d >= 0; d -= 5)
        {
            tca0_split_set_lcmp0_duty_percent((uint8_t)d);
            _delay_ms(50); // crude delay
        }
    }
}


