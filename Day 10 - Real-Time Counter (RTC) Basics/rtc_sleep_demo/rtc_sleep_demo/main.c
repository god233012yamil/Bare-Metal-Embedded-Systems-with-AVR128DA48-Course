/*
 * AVR128DA48 RTC Sleep Wake-up Demonstration
 * 
 * This project demonstrates using the Real-Time Counter (RTC) to periodically
 * wake the MCU from sleep mode. The MCU enters Power-Down sleep mode and
 * wakes up every second via RTC overflow interrupt to toggle an LED.
 * 
 * Hardware: AVR128DA48 Curiosity Nano
 * LED: Connected to PC6 (on-board LED)
 * 
 * Author: Created for embedded development demonstration
 * Date: 2026
 */

#define F_CPU 4000000UL  // 4 MHz default clock (OSCHF prescaled by 6)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

// LED pin definition (PC6 on Curiosity Nano)
#define LED_PIN PIN6_bm
#define LED_PORT PORTC

// RTC period for 1 second wake-up (using 32.768 kHz / 32 prescaler = 1024 Hz)
#define RTC_PERIOD 1024  // 1024 ticks = 1 second

/**
 * @brief Initialize the system clock
 * 
 * Configures the main clock to use the internal 24 MHz oscillator
 * with a prescaler of 6, resulting in a 4 MHz system clock.
 * This is the default configuration, but explicitly set for clarity.
 */
void clock_init(void)
{
    // The AVR128DA48 boots with OSCHF at 4 MHz (24 MHz / 6 prescaler)
    // This is already the default, so no changes needed
    // If you want to change the clock:
    // 1. Write 0x9D to CCP (Configuration Change Protection)
    // 2. Write to CLKCTRL_MCLKCTRLB within 4 CPU cycles
    
    // For this demo, we use the default 4 MHz clock
}

/**
 * @brief Initialize GPIO for LED control
 * 
 * Configures PC6 as an output pin for the on-board LED.
 * The LED is active-low on the Curiosity Nano board.
 */
void gpio_init(void)
{
    // Set PC6 as output for LED
    LED_PORT.DIRSET = LED_PIN;
    
    // Turn LED off initially (high = off for active-low LED)
    LED_PORT.OUTSET = LED_PIN;
}

/**
 * @brief Initialize the Real-Time Counter (RTC)
 * 
 * Configures the RTC to use the internal 32.768 kHz oscillator (OSC32K)
 * with a prescaler of 32, resulting in a 1024 Hz tick rate.
 * Sets up periodic interrupt every 1024 ticks (1 second).
 * 
 * The RTC continues running in all sleep modes, making it ideal
 * for wake-up timing in low-power applications.
 */
void rtc_init(void)
{
    // Wait for all registers to be ready
    while (RTC.STATUS > 0);
    
    // Select 32.768 kHz internal oscillator as RTC clock source
    // First, enable the internal 32 kHz oscillator
    CCP = CCP_IOREG_gc;  // Unlock protected register
    CLKCTRL.OSC32KCTRLA = CLKCTRL_RUNSTDBY_bm;  // Enable in all modes
    
    // Wait for the oscillator to stabilize
    _delay_ms(10);
    
    // Disable RTC while configuring
    RTC.CTRLA = 0;
    
    // Configure RTC clock source
    // 32.768 kHz / 32 = 1024 Hz tick rate
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;  // Select 32.768 kHz from OSC32K
    
    // Set period for 1 second (1024 ticks at 1024 Hz)
    RTC.PER = RTC_PERIOD - 1;
    
    // Clear counter
    RTC.CNT = 0;
    
    // Enable overflow interrupt
    RTC.INTCTRL = RTC_OVF_bm;
    
    // Enable RTC with prescaler DIV32 and RUN in standby
    RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;
    
    // Wait for synchronization
    while (RTC.STATUS & RTC_PERBUSY_bm);
}

/**
 * @brief Initialize sleep mode configuration
 * 
 * Configures the Sleep Controller (SLPCTRL) for Power-Down mode.
 * In Power-Down mode:
 * - Main clock is stopped
 * - RTC continues running (with RUNSTDBY enabled)
 * - Wake-up possible from enabled interrupts
 * - Lowest power consumption while maintaining RTC
 */
void sleep_init(void)
{
    // Configure sleep mode to Power-Down
    // Available modes:
    // - IDLE: CPU stopped, peripherals running
    // - STANDBY: Most peripherals stopped, wake-up peripherals active
    // - POWER_DOWN: Deepest sleep, only wake-up sources active
    SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc | SLPCTRL_SEN_bm;
}

/**
 * @brief RTC Overflow Interrupt Service Routine
 * 
 * This ISR is called every time the RTC counter overflows (every 1 second).
 * It toggles the LED to provide visual feedback that the MCU has woken up.
 * After the ISR completes, the MCU will return to sleep mode automatically
 * when the main loop executes sleep_cpu().
 */
ISR(RTC_CNT_vect)
{
    // Clear the overflow interrupt flag
    RTC.INTFLAGS = RTC_OVF_bm;
    
    // Toggle LED to indicate wake-up
    LED_PORT.OUTTGL = LED_PIN;
}

/**
 * @brief Main program entry point
 * 
 * Initializes all peripherals and enters an infinite loop that
 * puts the MCU to sleep. The RTC interrupt wakes the MCU every second,
 * toggles the LED, and then returns to sleep.
 * 
 * Power consumption flow:
 * 1. MCU enters Power-Down sleep (~5-10 µA typical)
 * 2. RTC overflow interrupt fires (every 1 second)
 * 3. MCU wakes, executes ISR (~1 ms active)
 * 4. Returns to sleep automatically
 * 
 * @return Never returns (infinite loop)
 */
int main(void)
{
    // Initialize all subsystems
    clock_init();
    gpio_init();
    rtc_init();
    sleep_init();
    
    // Enable global interrupts
    sei();
    
    // Brief startup indication - blink LED 3 times
    for (uint8_t i = 0; i < 3; i++)
    {
        LED_PORT.OUTCLR = LED_PIN;  // LED on
        _delay_ms(100);
        LED_PORT.OUTSET = LED_PIN;  // LED off
        _delay_ms(100);
    }
    
    // Main loop - just sleep and wake on RTC interrupt
    while (1)
    {
        // Enter sleep mode
        // The MCU will wake up when the RTC overflow interrupt fires
        // After servicing the interrupt, it returns here and sleeps again
        sleep_cpu();
        
        // Optional: Add a small active period after wake-up
        // This can be useful for doing work after wake-up
        // The ISR already toggles the LED, but you could do more here
    }
}