#include <avr/io.h>
#include <avr/cpufunc.h>   /* ccp_write_io() */

// Define the LED pin and port.
#define LED_PORT        PORTC
#define LED_PIN_bm      PIN6_bm 

/**
 * @brief Initialize the LED pin as an output.
 */
static void led_init(void)
{
    // Set the LED pin as an output.
    LED_PORT.DIRSET = LED_PIN_bm;   
}

/**
 * @brief A simple delay function.
 */
static void delay(void)
{
    for (volatile uint32_t i = 0; i < 300000; i++)
    {
    }
}

/**
 * @brief Initialize the watchdog timer.
 */
static void wdt_init(void)
{
    // Write the desired period; window mode is 
    //left OFF (bits 7:4 = 0)
    ccp_write_io((void *)&WDT.CTRLA, WDT_PERIOD_1KCLK_gc);

    // Wait until the register value has 
    // synchronized to the ULP domain 
    while (WDT.STATUS & WDT_SYNCBUSY_bm)
    {
        /* spin — takes a few ULP cycles 
        (~few µs at 4 MHz CPU) */
    }
}

/**
 * @brief The main function.
 */
int main(void)
{
    // Initialize the LED pin as an output
    LED_init();
    
    // Initialize the watchdog timer
    WDT_init();

    while (1)
    {
        // Toggle the LED state
        LED_PORT.OUTTGL = LED_PIN_bm;

        // Wait for a short delay (you can adjust this as needed)
        delay();

        // Reset the watchdog timer to prevent a system reset
        __asm__ __volatile__("wdr");
    }
}