/*
 * AVR128DA48 RTC and Sleep Configuration Reference
 * 
 * This file provides detailed register configuration information
 * and alternative configurations for the RTC and Sleep Controller.
 * Use this as a reference when customizing the main application.
 */

#ifndef RTC_SLEEP_CONFIG_H_
#define RTC_SLEEP_CONFIG_H_

/*
 * ============================================================================
 * RTC CLOCK SOURCE OPTIONS
 * ============================================================================
 */

// Option 1: Internal 32.768 kHz oscillator from OSC32K (Used in main code)
// Pros: No external components, always available
// Cons: Less accurate than external crystal (~3% tolerance)
#define RTC_CLK_OSC32K    RTC_CLKSEL_OSC32K_gc

// Option 2: Internal 1.024 kHz oscillator from OSC32K
// Pros: Lower power, no external components
// Cons: Lower frequency limits resolution
#define RTC_CLK_OSC1K     RTC_CLKSEL_OSC1K_gc

// Option 3: External 32.768 kHz crystal on XTAL32K pins
// Pros: High accuracy (±20 ppm typical)
// Cons: Requires external crystal and load capacitors
#define RTC_CLK_XOSC32K   RTC_CLKSEL_XOSC32K_gc

// Option 4: External clock on EXTCLK pin
// Pros: Can use any external clock source
// Cons: Requires external oscillator circuit
#define RTC_CLK_EXTCLK    RTC_CLKSEL_EXTCLK_gc

/*
 * ============================================================================
 * RTC PRESCALER OPTIONS
 * ============================================================================
 * 
 * Base frequency: 32.768 kHz = 32768 Hz
 * 
 * Prescaler  | Output Freq | Period per tick | Example: 1 sec
 * -----------|-------------|-----------------|-------------------
 * DIV1       | 32768 Hz    | 30.5 µs         | PER = 32767
 * DIV2       | 16384 Hz    | 61.0 µs         | PER = 16383
 * DIV4       | 8192 Hz     | 122 µs          | PER = 8191
 * DIV8       | 4096 Hz     | 244 µs          | PER = 4095
 * DIV16      | 2048 Hz     | 488 µs          | PER = 2047
 * DIV32      | 1024 Hz     | 977 µs          | PER = 1023  ? Used in demo
 * DIV64      | 512 Hz      | 1.95 ms         | PER = 511
 * DIV128     | 256 Hz      | 3.91 ms         | PER = 255
 * DIV256     | 128 Hz      | 7.81 ms         | PER = 127
 * DIV512     | 64 Hz       | 15.6 ms         | PER = 63
 * DIV1024    | 32 Hz       | 31.2 ms         | PER = 31
 * DIV2048    | 16 Hz       | 62.5 ms         | PER = 15
 * DIV4096    | 8 Hz        | 125 ms          | PER = 7
 * DIV8192    | 4 Hz        | 250 ms          | PER = 3
 * DIV16384   | 2 Hz        | 500 ms          | PER = 1
 * DIV32768   | 1 Hz        | 1 second        | PER = 0
 */

// Common prescaler configurations
#define RTC_PRESCALER_1HZ     RTC_PRESCALER_DIV32768_gc  // Slowest, 1 Hz
#define RTC_PRESCALER_1024HZ  RTC_PRESCALER_DIV32_gc     // Used in demo
#define RTC_PRESCALER_32768HZ RTC_PRESCALER_DIV1_gc      // Fastest, no division

/*
 * ============================================================================
 * SLEEP MODE CONFIGURATIONS
 * ============================================================================
 */

/*
 * IDLE MODE
 * ---------
 * CPU: OFF
 * Main clock: ON
 * Peripherals: ON
 * Interrupt wake: YES (any enabled interrupt)
 * Typical current: ~2 mA @ 4 MHz
 * Wake-up time: Fastest (immediate)
 * Use case: Short delays with peripheral activity
 */
#define SLEEP_MODE_IDLE \
    (SLPCTRL_SMODE_IDLE_gc | SLPCTRL_SEN_bm)

/*
 * STANDBY MODE
 * ------------
 * CPU: OFF
 * Main clock: OFF
 * Peripherals: Selected (with RUNSTDBY bit set)
 * Interrupt wake: YES (from enabled wake sources)
 * Typical current: ~50 µA
 * Wake-up time: Fast (~6 µs)
 * Use case: Moderate power saving with selected peripherals
 */
#define SLEEP_MODE_STANDBY \
    (SLPCTRL_SMODE_STDBY_gc | SLPCTRL_SEN_bm)

/*
 * POWER-DOWN MODE
 * ---------------
 * CPU: OFF
 * Main clock: OFF
 * Peripherals: OFF (except RTC, WDT, CCL with async clock)
 * Interrupt wake: YES (limited sources: RTC, PIN, WDT)
 * Typical current: ~5-10 µA
 * Wake-up time: Slow (~10 µs)
 * Use case: Maximum power saving, periodic wake-up
 */
#define SLEEP_MODE_POWER_DOWN \
    (SLPCTRL_SMODE_PDOWN_gc | SLPCTRL_SEN_bm)

/*
 * ============================================================================
 * TYPICAL RTC CONFIGURATIONS
 * ============================================================================
 */

/**
 * Configuration 1: 1-second wake-up (Used in demo)
 * Clock: 32.768 kHz / 32 = 1024 Hz
 * Period: 1024 ticks = 1 second
 */
#define RTC_CONFIG_1SEC_WAKEUP() do { \
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; \
    RTC.PER = 1023; \
    RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm; \
} while(0)

/**
 * Configuration 2: 100ms wake-up (10 Hz)
 * Clock: 32.768 kHz / 32 = 1024 Hz
 * Period: 102 ticks ? 100ms (actual: 99.6 ms)
 */
#define RTC_CONFIG_100MS_WAKEUP() do { \
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; \
    RTC.PER = 101; \
    RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm; \
} while(0)

/**
 * Configuration 3: 10-second wake-up
 * Clock: 32.768 kHz / 32 = 1024 Hz
 * Period: 10240 ticks = 10 seconds (requires 16-bit counter)
 */
#define RTC_CONFIG_10SEC_WAKEUP() do { \
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; \
    RTC.PER = 10239; \
    RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm; \
} while(0)

/**
 * Configuration 4: Maximum period wake-up
 * Clock: 32.768 kHz / 32768 = 1 Hz
 * Period: 65535 ticks = 65535 seconds ? 18.2 hours
 */
#define RTC_CONFIG_MAX_PERIOD() do { \
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc; \
    RTC.PER = 65535; \
    RTC.CTRLA = RTC_PRESCALER_DIV32768_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm; \
} while(0)

/*
 * ============================================================================
 * RTC INTERRUPT OPTIONS
 * ============================================================================
 */

// Overflow interrupt (fires when CNT reaches PER)
#define RTC_INT_OVERFLOW    RTC_OVF_bm

// Compare interrupt (fires when CNT matches CMP)
#define RTC_INT_COMPARE     RTC_CMP_bm

// Both interrupts
#define RTC_INT_BOTH        (RTC_OVF_bm | RTC_CMP_bm)

/*
 * ============================================================================
 * PERIPHERAL RUNSTDBY CONFIGURATION
 * ============================================================================
 * 
 * To keep peripherals running in Standby mode, set their RUNSTDBY bit:
 */

// Enable RTC in standby (required for sleep wake-up)
#define RTC_ENABLE_RUNSTDBY() \
    (RTC.CTRLA |= RTC_RUNSTDBY_bm)

// Enable USART in standby (for serial communication wake-up)
#define USART0_ENABLE_RUNSTDBY() \
    (USART0.CTRLB |= USART_RXMODE_NORMAL_gc)

// Enable ADC in standby
#define ADC0_ENABLE_RUNSTDBY() \
    (ADC0.CTRLA |= ADC_RUNSTBY_bm)

/*
 * ============================================================================
 * POWER REDUCTION TECHNIQUES
 * ============================================================================
 */

/**
 * Disable unused peripherals to save power
 * Call this before entering sleep mode
 */
static inline void disable_unused_peripherals(void)
{
    // Disable ADC
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
    
    // Disable DAC
    DAC0.CTRLA &= ~DAC_ENABLE_bm;
    
    // Disable AC (Analog Comparator)
    AC0.CTRLA &= ~AC_ENABLE_bm;
    
    // Disable all USARTs if not needed
    USART0.CTRLB &= ~USART_RXEN_bm;
    USART0.CTRLB &= ~USART_TXEN_bm;
    USART1.CTRLB &= ~USART_RXEN_bm;
    USART1.CTRLB &= ~USART_TXEN_bm;
    
    // Disable TWI if not needed
    TWI0.MCTRLA &= ~TWI_ENABLE_bm;
    
    // Disable SPI if not needed
    SPI0.CTRLA &= ~SPI_ENABLE_bm;
}

/**
 * Configure all pins to minimize current draw
 * Floating pins can cause significant current consumption
 */
static inline void configure_pins_for_low_power(void)
{
    // Configure all unused pins as inputs with pull-up
    // This prevents floating inputs which can cause current draw
    
    // Port A - configure unused pins
    PORTA.PIN0CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
    // ... configure all unused pins
    
    // Alternative: Set unused pins as outputs and drive low
    // PORTA.DIRSET = 0xFF;  // All outputs
    // PORTA.OUTCLR = 0xFF;  // All low
}

/*
 * ============================================================================
 * EXAMPLE: Using RTC Compare for Multiple Wake-ups
 * ============================================================================
 */

/**
 * Example: Wake every 1 second (OVF) AND at 500ms (CMP)
 * This allows two different wake-up points within each period
 * 
 * Note: The AVR128DA48 RTC only has one interrupt vector (RTC_CNT_vect)
 * that handles both overflow and compare interrupts. You need to check
 * the INTFLAGS register to determine which event occurred.
 */
void rtc_init_dual_wakeup(void)
{
    while (RTC.STATUS > 0);
    
    RTC.CTRLA = 0;  // Disable
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;
    RTC.PER = 1023;      // 1 second period
    RTC.CMP = 511;       // Compare at 500ms
    RTC.CNT = 0;
    
    // Enable both overflow and compare interrupts
    RTC.INTCTRL = RTC_OVF_bm | RTC_CMP_bm;
    
    RTC.CTRLA = RTC_PRESCALER_DIV32_gc | RTC_RTCEN_bm | RTC_RUNSTDBY_bm;
    while (RTC.STATUS & RTC_PERBUSY_bm);
}

// ISR for RTC - handles both overflow and compare
ISR(RTC_CNT_vect)
{
    // Check which interrupt occurred
    if (RTC.INTFLAGS & RTC_OVF_bm) {
        RTC.INTFLAGS = RTC_OVF_bm;  // Clear overflow flag
        // Handle overflow (1 second) wake-up
    }
    
    if (RTC.INTFLAGS & RTC_CMP_bm) {
        RTC.INTFLAGS = RTC_CMP_bm;  // Clear compare flag
        // Handle compare (500ms) wake-up
    }
}

/*
 * ============================================================================
 * DEBUGGING TIPS
 * ============================================================================
 */

/*
 * 1. Debug in sleep modes:
 *    - UPDI keeps the CPU running by default during debug
 *    - To test actual sleep: disconnect debugger or disable UPDI fuse
 * 
 * 2. Measure actual current:
 *    - Remove jumper J101 on Curiosity Nano
 *    - Insert ammeter between J101 pins
 *    - Observe sleep vs. wake current
 * 
 * 3. RTC not running:
 *    - Check OSC32K is enabled (CLKCTRL.OSC32KCTRLA)
 *    - Verify RUNSTDBY bit is set
 *    - Allow sufficient startup time for oscillator
 * 
 * 4. Unexpected wake-ups:
 *    - Check all interrupt sources
 *    - Verify INTCTRL registers
 *    - Clear all INTFLAGS before sleep
 * 
 * 5. Higher than expected current:
 *    - Check for floating pins
 *    - Verify unused peripherals are disabled
 *    - Check for unintended pull-ups/pull-downs
 */

#endif /* RTC_SLEEP_CONFIG_H_ */