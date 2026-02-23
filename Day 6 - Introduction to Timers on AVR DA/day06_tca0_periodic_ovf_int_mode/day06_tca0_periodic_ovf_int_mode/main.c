/*
 * AVR128DA48 - TCA0 Overflow Interrupt: 1 ms tick
 *
 * Clock:
 * - OSCHF = 24 MHz
 * - Main prescaler /2 => CLK_PER = 12 MHz
 *
 * TCA0:
 * - Prescaler DIV8 => TCA clock = 12 MHz / 8 = 1.5 MHz
 * - PER = 1499 => overflow every 1500 ticks
 * - 1500 / 1.5 MHz = 0.001 s = 1 ms
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

// Curiosity Nano LED: PC6, active-low
#define LED_PORT   PORTC
#define LED_PIN_bm PIN6_bm

// Global 1 ms tick counter (increments in ISR)
static volatile uint32_t g_ovf_hits = 0;

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
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLA,
                 (uint8_t)(CLKCTRL_CLKSEL_OSCHF_gc | (0 << CLKCTRL_CLKOUT_bp)));

    // Set OSCHF frequency: 24 MHz 
    ccp_write_io((volatile uint8_t *)&CLKCTRL.OSCHFCTRLA,
                 (uint8_t)(CLKCTRL_FRQSEL_24M_gc |
                           (0 << CLKCTRL_AUTOTUNE_bp) |
                           (0 << CLKCTRL_RUNSTDBY_bp)));

    // Enable prescaler, divide by 2 (CLK_PER = 12 MHz) 
    ccp_write_io((volatile uint8_t *)&CLKCTRL.MCLKCTRLB,
                 (uint8_t)(CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc));
}

/*
 * Configure TCA0 to overflow every 1 ms and fire OVF interrupt.
 */
static void tca0_init_1ms_ovf_irq(void)
{
    // Stop timer while configuring 
    TCA0.SINGLE.CTRLA &= ~(1 << TCA_SINGLE_ENABLE_bm);   

    // Normal mode (count up to PER, then overflow) 
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;

    // Set TOP for 1 ms overflow 
    TCA0.SINGLE.PER = 1499;

    // Clear any pending OVF flag 
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;

    // Enable overflow interrupt 
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;

    // Start timer: prescaler DIV8, enable 
    TCA0.SINGLE.CTRLA = (uint8_t)(TCA_SINGLE_CLKSEL_DIV8_gc | TCA_SINGLE_ENABLE_bm);
}

/*
 * TCA0 overflow ISR
 */
ISR(TCA0_OVF_vect)
{
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;   // Clear OVF flag
    g_ovf_hits++;								// 1 ms tick counter 
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

// Application entry point
int main(void)
{
    // Configure system clock
	clock_init_24mhz_presc2();
	
	// Configure PLED pin
    led_init();
	
	// Configure TCA0 to overflow every 1 ms
    tca0_init_1ms_ovf_irq();

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
        if (TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) {
			TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;   // clear flag		
			g_ovf_hits++;			
        }
		
		if(g_ovf_hits >= 500) {
			LED_PORT.OUTTGL = LED_PIN_bm; // atomic toggle
			g_ovf_hits = 0;
		}		
		
    }
}