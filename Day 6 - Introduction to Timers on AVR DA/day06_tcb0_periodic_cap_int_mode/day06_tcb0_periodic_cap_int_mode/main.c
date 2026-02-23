/*
 * AVR128DA48 - TCB Periodic Interrupt Mode: 1 ms tick
 *
 * Clock assumption:
 * - OSC24M = 24 MHz
 * - CLK_PER = 24 MHz / 2 = 12 MHz
 *
 * TCB clock selection:
 * - CLK_PER/2 -> 6 MHz timer clock
 *
 * 1 ms tick:
 * - 6 MHz * 1 ms = 6000 counts
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Curiosity Nano LED: PC6, active-low
#define LED_PORT   PORTC
#define LED_PIN_bm PIN6_bm

// Toggle LED every N milliseconds 
#define LED_TOGGLE_MS       500U

// Global 1 ms tick counter (increments in ISR)
static volatile uint32_t g_ms_ticks = 0;

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
 * - Enable prescaler /2 -> CLK_PER = 12 MHz
 */
static void clock_init_24mhz_presc2(void)
{
    // Select main clock source: OSCHF, CLKOUT disabled
	ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA), 
						   CLKCTRL_CLKSEL_OSCHF_gc 
					| 0 << CLKCTRL_CLKOUT_bp);
					
	// Set OSCHF frequency to 24 MHz, AUTOTUNE disabled, RUNSTDBY disabled
	ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA), CLKCTRL_FRQSEL_24M_gc 
					| 0 << CLKCTRL_AUTOTUNE_bp 
					| 0 << CLKCTRL_RUNSTDBY_bp);
					
	// Enable main prescaler, divide by 2
    ccp_write_io(&CLKCTRL.MCLKCTRLB, CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc);
}

/*
 * Initialize TCB0 in Periodic Interrupt Mode for a 1 ms tick.
 *
 * With CLK_PER = 12 MHz and TCB clock = CLK_PER/2:
 * - TCB clock = 6 MHz
 * - CCMP = 6000 -> 1 ms period
 */
static void tcb0_init_1ms_tick(void)
{
    // Disable TCB0 before configuration
    TCB0.CTRLA &= ~(1 << TCB_ENABLE_bp);          
	                      
	// CCMP calculation:
	// CCMP = (time_in_ms * F_CPU) / 1000
	// Period for 1 ms (6 MHz / 1000) 
    TCB0.CCMP = 6000;
	
	// reset counter (good practice)
	TCB0.CNT = 0U;

    // Periodic interrupt mode 
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;

    // Clear pending interrupt flag
    TCB0.INTFLAGS = TCB_CAPT_bm;

    // Enable CAPT interrupt, disable overflow interrupt
    TCB0.INTCTRL = 1 << TCB_CAPT_bp		// Capture/Timeout interrupt enabled
				 | 0 << TCB_OVF_bp;		// Overflow interrupt disabled

    // Start timer: clock = CLK_PER/2, enable
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc 
	           | TCB_ENABLE_bm;
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
static void led_init(void)
{
	// Set PC6 as output
	LED_PORT.DIRSET = LED_PIN_bm;
	
	// Drive PC6 high to turn LED off at start,
	// because the lED is active-low
	LED_PORT.OUTSET  = LED_PIN_bm;
}

/*
 * Application entry point
 */
int main(void)
{
    // Configure system clock
	clock_init_24mhz_presc2();
	
	// Configure PLED pin
	led_init();
	
	// Initialize TCB0 in Periodic Interrupt Mode for a 1 ms tick.
    tcb0_init_1ms_tick();
	
	// Enable global interrupts.
    sei();

    while (1)
    {
        /*
         * Polling probe (diagnostic):
         * If this toggles the LED but the ISR breakpoint never hits,
         * then TCB is running, but interrupts are not being serviced.
         *
         * Comment this block out after debugging.
         */
        if (TCB0.INTFLAGS & TCB_CAPT_bm) {
			TCB0.INTFLAGS = TCB_CAPT_bm;    // clear flag
			g_ms_ticks++;					// 1 ms tick counter					
        }
		
		// Toggle LED every N milliseconds
		if(g_ms_ticks >= LED_TOGGLE_MS) {
			LED_PORT.OUTTGL = LED_PIN_bm;   // atomic toggle
			g_ms_ticks = 0;
		}
		
    }
	
	return 0; /* Never reached */
}