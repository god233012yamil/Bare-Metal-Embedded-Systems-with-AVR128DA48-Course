#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Curiosity Nano LED: PC6, active-low
#define LED_PORT   PORTC
#define LED_PIN_bm PIN6_bm

// Toggle LED every N milliseconds
#define LED_TOGGLE_MS       500U

// Debug counter: increments every 1 ms if OVF ISR runs 
static volatile uint32_t g_ovf_isr_hits = 0;

// Preload so overflow occurs after 6000 ticks (1 ms at 6 MHz) 
// Calculation:
// ticks_for_1ms = (time_in_ms * F_CPU) / 1000
// ticks_for_1ms = (6 MHz / 1000) = 6000
// CNT_start = 65536 - ticks_for_1ms
// CNT_start = 65536 - 6000 = 59536 
#define TCB0_PRELOAD_1MS  (uint16_t)59536

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
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLA,
                 (uint8_t)(CLKCTRL_CLKSEL_OSCHF_gc | (0 << CLKCTRL_CLKOUT_bp)));

    // Set OSCHF frequency: 24 MHz, AUTOTUNE disabled, RUNSTDBY disabled
    ccp_write_io((volatile uint8_t *)&CLKCTRL.OSCHFCTRLA,
                 (uint8_t)(CLKCTRL_FRQSEL_24M_gc |
                           (0 << CLKCTRL_AUTOTUNE_bp) |
                           (0 << CLKCTRL_RUNSTDBY_bp)));

    // Enable prescaler, divide by 2
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLB,
                 (uint8_t)(CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc));
}

/*
 * Configure TCB0 so OVF interrupt fires every 1 ms.
 *
 * Technique:
 * - CLK_PER = 12 MHz, then TCB0 run at CLK_PER/2 = 6 MHz
 * - Preload CNT to 0xE890 so it overflows after 6000 counts (1 ms)
 * - Enable OVF interrupt
 * - Reload CNT in ISR
 */
static void tcb0_init_1ms_ovf_irq(void)
{
    TCB0.CTRLA = 0;								// Stop timer while configuring

    // Use a basic counting mode; we do not rely on CAPT/CCMP events here.
    // Periodic Interrupt mode is fine as long as we enable OVF and reload CNT.
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;			// Mode selection (not using CAPT IRQ)

    TCB0.CCMP = 0xFFFF;							// Avoid early compare behavior; overflow is our event    
	TCB0.CNT  = TCB0_PRELOAD_1MS;				// Preload for 1 ms overflow

    TCB0.INTFLAGS = (TCB_OVF_bm | TCB_CAPT_bm); // Clear any pending flags
    TCB0.INTCTRL  = TCB_OVF_bm;					// Enable overflow interrupt only

    TCB0.CTRLA = (uint8_t)(TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm); // Start: CLK_PER/2
}

/*
 * TCB0 interrupt vector (handles OVF and/or CAPT depending on what you enable)
 */
ISR(TCB0_INT_vect)
{
    if (TCB0.INTFLAGS & TCB_OVF_bm) {
	    TCB0.INTFLAGS = TCB_OVF_bm;   // Clear OVF flag
	    TCB0.CNT = TCB0_PRELOAD_1MS;  // Reload for next 1 ms period
	    g_ovf_isr_hits++;             // 1 ms tick counter
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
	
	// Configure TCB0 so OVF interrupt fires every 1 ms.
    tcb0_init_1ms_ovf_irq();

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
		if (TCB0.INTFLAGS & TCB_OVF_bm) {
			TCB0.INTFLAGS = TCB_OVF_bm;   // Clear OVF flag
			TCB0.CNT = TCB0_PRELOAD_1MS;  // Reload for next 1 ms period			
			g_ovf_isr_hits++;             // 1 ms tick counter			
		}
		
		// Toggle LED every N milliseconds
		if(g_ovf_isr_hits >= 500) {
			LED_PORT.OUTTGL = LED_PIN_bm; // atomic toggle
			g_ovf_isr_hits = 0;
		}		
		
    }
}