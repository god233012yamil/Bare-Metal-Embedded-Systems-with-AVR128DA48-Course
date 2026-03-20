#include <avr/io.h>
#include <avr/interrupt.h>

// Global variable to store the latest ADC value
volatile uint16_t adc_value;

// Define LED pin and port (PC6 = LED0 on Curiosity Nano)
#define LED_PORT PORTC
#define LED_PIN  PIN6_bm 

/**
 * @brief Initialize the LED pin as output, RTC for 1 second 
 * overflow interrupts, and ADC0 to read from PA0 with 
 * internal 2.048 V reference.
 */
static void LED_init(void)
{
    // Set LED pin as output
    LED_PORT.DIRSET = LED_PIN;
}

/**
 * @brief Initialize the RTC to generate an overflow interrupt every 1 second.
 */
static void RTC_init(void)
{
    // Configure RTC for 1 second overflow interrupts.
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    // Set the period to 32768, which corresponds 
    //to 1 second at 32.768 kHz.
    RTC.PER = 32768;

    // Enable overflow interrupt.
    RTC.INTCTRL = RTC_OVF_bm;

    // Enable the RTC.
    RTC.CTRLA = RTC_RTCEN_bm;
}

/**
 * @brief Initialize the ADC0 to read from PA0 with internal 2.048 V reference.
 */
static void ADC0_init(void)
{
    // Configure PD0 as input, disable digital 
    // input buffer to save power
    PORTD.DIRCLR    = PIN0_bm;
    PORTD.PIN0CTRL  = PORT_ISC_INPUT_DISABLE_gc;

    // Set voltage reference for ADC to internal 2.048 V
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;

    // Clock prescaler: 
    // DIV20 -> 20 MHz / 20 = 1.000 MHz (within spec)
    ADC0.CTRLC = ADC_PRESC_DIV20_gc;

    // Select the positive input for ADC0 as AIN0 (PA0).
    ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;

    // Enable the ADC interrupt on result ready.
    ADC0.INTCTRL = ADC_RESRDY_bm;

    // Enable the ADC.
    ADC0.CTRLA = ADC_ENABLE_bm;
}

/**
 * @brief Interrupt service routine for RTC overflow interrupt.
 */
ISR(RTC_CNT_vect)
{
    // Clear the interrupt flag by writing a one to it.
    RTC.INTFLAGS = RTC_OVF_bm;

    // Toggle the LED.
    LED_PORT.OUTTGL = LED_PIN;

    // Start an ADC conversion.
    ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief Interrupt service routine for ADC0 result ready interrupt.
 */
ISR(ADC0_RESRDY_vect)
{
    // Clear the interrupt flag.
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    // Read the ADC value.
    adc_value = ADC0.RES;
}

/**
 * @brief Main function.
 */
int main(void) {
    // Initialize LED
    LED_init();
    
    // Initialize RTC
    RTC_init();

    // Initialize ADC0
    ADC0_init();

    // Enable global interrupts
    sei();

    while (1)
    {
        /* System mostly idle */
    }
}