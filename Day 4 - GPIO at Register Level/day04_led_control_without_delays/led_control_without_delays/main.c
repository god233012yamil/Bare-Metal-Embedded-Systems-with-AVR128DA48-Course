/*
 * Lab 10 - LED Control Without Delays
 *
 * Goal:
 * - Prove GPIO configuration
 * - Prove CPU execution
 * - Prove system clock is running
 *
 * Technique:
 * - Configure LED pin as output
 * - Toggle pin continuously using raw instruction flow
 *
 * No timers
 * No delay loops
 * No frameworks
 */

#include <avr/io.h>
#include <stdint.h>

/*
 * This value is empirical.
 * Change it and observe how blink speed changes.
 * It directly depends on CPU clock and compiler optimization.
 */
#define BLINK_TICKS  200000UL

/* ---------------- LED definition (from board schematic) ---------------- */

/* AVR128DA48 Curiosity Nano:
 * LED is on PC6
 * LED is active-low
 */
#define LED_PORT    PORTC
#define LED_PIN_bm  PIN6_bm

/* ---------------- LED control helpers ---------------- */

/*
 * Configure the pin PC6
 */
static inline void led_init(void) {
    LED_PORT.DIRSET = LED_PIN_bm;     // Configure PC6 as output
    LED_PORT.OUTSET = LED_PIN_bm;     // LED OFF (active-low, known startup state)
}

/*
 * Atomic toggle
 */
static inline void led_toggle(void) {
    LED_PORT.OUTTGL = LED_PIN_bm;     
}

/* ---------------- Main application ---------------- */

int main(void) {
	
    uint32_t tick_counter = 0;
	
	// Initialize the LED pin PC6
	led_init();

    while (1) {
		
		tick_counter++;

		if (tick_counter >= BLINK_TICKS) {
			tick_counter = 0;
			led_toggle(); // Toggle LED after N iterations
		}
		
    }
}