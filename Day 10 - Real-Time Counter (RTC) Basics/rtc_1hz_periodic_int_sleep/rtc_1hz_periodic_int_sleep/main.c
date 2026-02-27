#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

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
 * Initialize RTC in Periodic Interrupt Mode.
 */

static void rtc_init_1hz(void)
{
	// Ensure RTC is disabled before configuration
	RTC.CTRLA = 0;

	// Enable periodic interrupt at 1 Hz
	RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc   /* 32768 cycles = 1 second */
	             | RTC_PITEN_bm;

	// Clear any pending interrupt flag
	RTC.PITINTFLAGS = RTC_PI_bm;

	// Enable periodic interrupt
	RTC.PITINTCTRL = RTC_PI_bm;
}

/*
 * RTC interrupt vector
 */
ISR(RTC_PIT_vect)
{
    // Clear interrupt flag
    RTC.PITINTFLAGS = RTC_PI_bm;
	
    // Toggle LED
    PORTC.OUTTGL = PIN6_bm;
}

/*
 * Application entry point
 */
int main(void)
{
	// Initialize the pin used by the LED.
	gpio_led_init();
	
	// Initialize RTC in Periodic Interrupt Mode.
	rtc_init_1hz();

	// Enable global interrupts.
	sei();
	
	// Set the sleep mode to idle mode.
	set_sleep_mode(SLEEP_MODE_IDLE);

	while (1)
	{
		/*
         * Polling probe (diagnostic)         
         * Comment this block out after debugging.
         */
/*        if (RTC.PITINTFLAGS & RTC_PI_bm) {
			// Clear interrupt flag
			RTC.PITINTFLAGS = RTC_PI_bm;
			// Toggle LED
			PORTC.OUTTGL = PIN6_bm;
        }*/	

		/* CPU wakes up here after RTC interrupt */
		sleep_mode();
		
	}
}