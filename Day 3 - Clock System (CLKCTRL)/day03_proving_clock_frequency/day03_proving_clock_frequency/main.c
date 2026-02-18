/*
 * AVR128DA48 bare-metal clock + GPIO toggle demo
 *
 * - Clock source: OSC24M (24 MHz internal oscillator)
 * - Main prescaler: /2  -> CLK_MAIN = 12 MHz (CPU/peripheral clock)
 * - GPIO: PC4 output
 * - Main loop: toggles PC4 as fast as possible
 */
#include <avr/io.h>
#include <stdint.h>

// Protected register write helper for AVR Dx (CCP = Configuration Change Protection) 
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;   // unlock protected IO regs for a few cycles 
    *addr = value;
}

/*
 * Configure system clock:
 * - Select OSCHF as main clock source
 * - Set OSCHF frequency to 24 MHz
 * - Enable prescaler /2 -> CLK_PER = 12 MHz
 */
static void clock_init_24mhz_presc2(void)
{
    // Select main clock source: OSCHF, CLKOUT enabled
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA), 
						   CLKCTRL_CLKSEL_OSCHF_gc 
					| 1 << CLKCTRL_CLKOUT_bp);
					
	// Set OSCHF frequency to 24 MHz, AUTOTUNE disabled, RUNSTDBY disabled
	ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA), CLKCTRL_FRQSEL_24M_gc 
					| 0 << CLKCTRL_AUTOTUNE_bp 
					| 0 << CLKCTRL_RUNSTDBY_bp);
					
	// Enable main prescaler, divide by 2
    ccp_write_io(&CLKCTRL.MCLKCTRLB, CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc);
}

/*
 * Configure PC4 as output (push-pull).
 */
static void gpio_init_pc4_output(void)
{
	// PC4 output 
	PORTC.DIRSET = PIN4_bm;

	// Optional: start low 
	PORTC.OUTCLR = PIN4_bm;
}

/*
 * Configure PA7 as output for CLKOUT
 */
static void clkout_pin_init(void)
{
    // Datasheet: CLKOUT is output on PA7 
    PORTA.DIRSET = PIN7_bm;     

    // Disable pull-up just to be explicit 
    PORTA.PIN6CTRL = 0;
}

// Application entry point
int main(void) {
	// Configure main clock
	clock_init_24mhz_presc2();
	
	// Configure PC4 as output
	gpio_init_pc4_output();
	
	// Configure PA6 as output for CLKOUT
	// Clock is now continuously output on PA6.
	clkout_pin_init();

	/* Toggle as fast as possible.
	 * Use OUTTGL to flip only the selected bit
	 * without read-modify-write.
	 */
	for (;;) 
	{
		PORTC.OUTTGL = PIN4_bm;		
		
	}
}