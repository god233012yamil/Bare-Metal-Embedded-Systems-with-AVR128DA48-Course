#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU_HZ        (24000000UL)
#define TICK_SEC		(0.001)	// 1 ms
#define TICK_HZ         (1UL / TICK_SEC) 
#define TCB_TICKS       (F_CPU_HZ / TICK_HZ)

static volatile uint32_t g_ms_ticks = 0;

/*
 * Initialize TCB0 in Periodic Interrupt Mode for a 1 ms tick.
 *
 * With CLK_PER = 24 MHz and main prescaler disabled:
 * - TCB clock = 24 MHz
 * - CCMP = 24000 -> 1 ms period
 */
static void tcb0_tick_init(void)
{
	// Disable TCB0 before configuration 
	TCB0.CTRLA &= ~(1 << TCB_ENABLE_bp);

	// Periodic interrupt mode 
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;

	// Set compare value for 1 ms interval
	TCB0.CCMP = (uint16_t)(TCB_TICKS - 1U);

	// Clear any pending interrupt flags 
	TCB0.INTFLAGS = TCB_CAPT_bm;

	// Enable capture/compare interrupt
	TCB0.INTCTRL = TCB_CAPT_bm;

	// Enable timer, use CLK_PER 
	TCB0.CTRLA = TCB_ENABLE_bm;
}

/*
 * TCB0 interrupt vector (handles OVF and/or CAPT depending on what you enable)
 */
ISR(TCB0_INT_vect)
{
	if (TCB0.INTFLAGS & TCB_CAPT_bm) {
		TCB0.INTFLAGS = TCB_CAPT_bm;  // clear interrupt flag
		g_ms_ticks++;				  // 1 ms tick counter
	}
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
	
	// Initializes TCB0 in Periodic interrupt mode. 
	tcb0_tick_init();
	
	// Enable global interrupts.
	sei();

	uint32_t last = 0;

	while (1)
	{
		/*
         * Polling probe (diagnostic):
         * If this toggles the LED but the ISR breakpoint never hits,
         * then TCB is running, but interrupts are not being serviced.
         *
         * Comment this block out after debugging.
         */
/*        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
			TCB0.INTFLAGS = TCB_CAPT_bm;    
			g_ms_ticks++;					// 1 ms tick counter					
        }*/		
		
		if ((g_ms_ticks - last) >= 500U)
		{
			last = g_ms_ticks;
			gpio_led_toggle();
		}
	}
}
