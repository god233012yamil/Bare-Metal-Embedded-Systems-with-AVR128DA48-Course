#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU_HZ        (24000000UL)

static volatile uint8_t g_oneshot_fired = 0;

/*
 * Initialize TCB1 in Periodic Interrupt Mode to fire one shot.
 */
static void tcb1_oneshot_start_us(uint16_t delay_us)
{
    /*
     * delay_us is in microseconds.
     * If CLK_PER is 24 MHz, then ticks per us = 24.
     * ticks = delay_us * 24
     */
    uint32_t ticks = (uint32_t)delay_us * (F_CPU_HZ / 1000000UL);

    if (ticks == 0)
    {
        ticks = 1;
    }

    if (ticks > 0xFFFFUL)
    {
        ticks = 0xFFFFUL; /* clamp */
    }

    g_oneshot_fired = 0;

    TCB1.CTRLA = 0;
    TCB1.CTRLB = TCB_CNTMODE_INT_gc;
    TCB1.CCMP = (uint16_t)(ticks - 1U);
    TCB1.INTFLAGS = TCB_CAPT_bm;
    TCB1.INTCTRL = TCB_CAPT_bm;

    // Enable and start
    TCB1.CTRLA = TCB_ENABLE_bm;
}

ISR(TCB1_INT_vect)
{
    // Clear flag 
    TCB1.INTFLAGS = TCB_CAPT_bm;
    // Stop timer to make it one-shot 
    TCB1.CTRLA &= ~TCB_ENABLE_bm;
    g_oneshot_fired = 1;
}

/*
 * Configure LED pin as output (push-pull).
 */
static void gpio_led_init(void)
{
	// Set PC6 as output
	PORTC.DIRSET = PIN6_bm;
	
	// Drive PC6 high to turn LED off at start,
	// because the lED is active-low
	PORTC.OUTSET = PIN6_bm;
}

/*
 * Toggle the LED
 */
static void gpio_led_toggle(void)
{
	PORTC.OUTTGL = PIN6_bm;
}

/*
 * CCP-protected register write helper
 */
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;
    *addr = value;
}

/*
 * Configure system clock:
 * - Select OSCHF as main clock source
 * - Set OSCHF frequency to 24 MHz
 * - Prescaler disabled
 */
static void clock_init_24mhz(void)
{
    // Select main clock source: OSCHF, CLKOUT disabled
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA), 
						   CLKCTRL_CLKSEL_OSCHF_gc 
					| 0 << CLKCTRL_CLKOUT_bp);
					
	// Set OSCHF frequency to 24 MHz, AUTOTUNE disabled, RUNSTDBY disabled
	ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA), CLKCTRL_FRQSEL_24M_gc 
					| 0 << CLKCTRL_AUTOTUNE_bp 
					| 0 << CLKCTRL_RUNSTDBY_bp);
					
	// Disable main prescaler
    ccp_write_io(&CLKCTRL.MCLKCTRLB, 0);
}


/*
 * Application entry point
 */
int main(void)
{
	// Configure system clock
	clock_init_24mhz();
	
	// Initialize the pin used by the LED.
	gpio_led_init();
	
	// Enable global interrupts.
	sei();

	// Start a 200000 us one-shot (200 ms)
	tcb1_oneshot_start_us(200000U); /* will clamp to 0xFFFF ticks in this simple demo */

	while (1)
	{
		// Polling probe (diagnostic) in case the ISR is not called
        if (TCB1.INTFLAGS & TCB_CAPT_bm) {
			// Clear flag
			TCB1.INTFLAGS = TCB_CAPT_bm;
			// Stop timer to make it one-shot
			TCB1.CTRLA &= ~TCB_ENABLE_bm;
			g_oneshot_fired = 1;					
        }		
		
		// Toggle the LED
		if (g_oneshot_fired) {
			g_oneshot_fired = 0;
			gpio_led_toggle();
		}
	}
}

