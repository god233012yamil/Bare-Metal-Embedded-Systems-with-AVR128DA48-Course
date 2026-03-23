/*
 * Lab: Watchdog System Recovery
 * Target: AVR128DA48 Curiosity Nano
 * IDE:    Atmel Studio 7 / Microchip Studio
 *         GCC C Executable Project
 *         AVR-Dx Device Pack 2.4.286
 *
 * Hardware:
 *   LED0 (active-low) on PC6 — AVR128DA48 Curiosity Nano board
 *
 * Lab Goal:
 *   1. Enable the WDT with a ~4-second timeout
 *   2. Toggle the LED periodically to show the firmware is alive
 *   3. Count toggle cycles; after N cycles simulate a firmware crash
 *      (infinite loop — stops kicking the WDT)
 *   4. The WDT fires, resets the MCU, and the LED restarts blinking
 *
 * Observable behaviour on hardware:
 *   Power-on:          LED blinks 5 times (normal operation)
 *   After 5 blinks:    LED goes solid ON — simulated "crash"
 *   ~4 seconds later:  MCU resets; LED restarts blinking
 *   The RSTCTRL.RSTFR register is read at startup so we can tell the
 *   difference between a POR and a WDT reset; this is reflected in the
 *   LED startup pattern (see below).
 *
 * LED startup pattern:
 *   Power-on reset  ? 1 slow blink  (PORF set)
 *   WDT reset       ? 3 rapid blinks (WDRF set)
 *
 * WDT key notes for AVR-Dx:
 *   - WDT.CTRLA is a protected I/O register; write via CCP (Configuration
 *     Change Protection) sequence using ccp_write_io().
 *   - The WDT runs from the 1 kHz ULP oscillator; "1KCLK" = ~1 s period.
 *   - Once LOCK bit in WDT.STATUS is set the WDT cannot be changed/stopped.
 *     We do NOT set LOCK in this lab so the debugger can still stop the MCU.
 *   - Kick the WDT with:  asm volatile("wdr");
 *
 * Registers used (from ioavr128da48.h):
 *   WDT.CTRLA        — WDT Control A  (period select)
 *   WDT.STATUS       — WDT Status     (SYNCBUSY, LOCK)
 *   RSTCTRL.RSTFR    — Reset Flags    (PORF, WDRF, SWRF, …)
 *   RSTCTRL.SWRR     — Software Reset Register
 *   PORTC.DIRSET     — Set PC6 as output
 *   PORTC.OUTSET     — Drive PC6 high (LED off, active-low)
 *   PORTC.OUTTGL     — Toggle PC6
 */

#include <avr/io.h>
#include <avr/cpufunc.h>   /* ccp_write_io() */
#include <util/delay.h>    /* _delay_ms()    */

/* ------------------------------------------------------------------ */
/*  Board-specific constants                                            */
/*  AVR128DA48 Curiosity Nano: LED0 = PC6, active-low                  */
/* ------------------------------------------------------------------ */
#define LED_PORT        PORTC
#define LED_PIN_bm      PIN6_bm   /* 0x40 */

/* ------------------------------------------------------------------ */
/*  WDT timeout selection                                               */
/*  WDT_PERIOD_4KCLK_gc  ? 4.1 s  (4096 ULP cycles @ ~1 kHz)          */
/* ------------------------------------------------------------------ */
#define WDT_TIMEOUT     WDT_PERIOD_4KCLK_gc

/* Number of LED toggles before the simulated crash */
#define CRASH_AFTER_TOGGLES  5u

/* Blink timing (milliseconds) — must be << WDT timeout */
#define BLINK_ON_MS     200u
#define BLINK_OFF_MS    300u

/* Short "indicator" blink used in the startup pattern */
#define INDICATOR_ON_MS  80u
#define INDICATOR_OFF_MS 150u

/* ------------------------------------------------------------------ */
/*  Low-level helpers                                                   */
/* ------------------------------------------------------------------ */

static inline void led_on(void)
{
    LED_PORT.OUTCLR = LED_PIN_bm;   /* active-low: clear = on  */
}

static inline void led_off(void)
{
    LED_PORT.OUTSET = LED_PIN_bm;   /* active-low: set   = off */
}

static inline void led_toggle(void)
{
    LED_PORT.OUTTGL = LED_PIN_bm;
}

/* Kick (reset) the watchdog counter */
static inline void wdt_kick(void)
{
    asm volatile("wdr");
}

/* ------------------------------------------------------------------ */
/*  WDT initialisation                                                  */
/*                                                                      */
/*  WDT.CTRLA layout (8-bit):                                           */
/*    [3:0]  PERIOD   — timeout period                                  */
/*    [7:4]  WINDOW   — open window (0 = window mode off)               */
/*                                                                      */
/*  The register is CCP-protected; we must use ccp_write_io().          */
/*  Wait for SYNCBUSY to clear so the write has propagated to the       */
/*  asynchronous ULP clock domain before we rely on the WDT being live. */
/* ------------------------------------------------------------------ */
static void wdt_init(void)
{
    /* Write the desired period; window mode is left OFF (bits 7:4 = 0) */
    ccp_write_io((void *)&WDT.CTRLA, WDT_TIMEOUT);

    /* Wait until the register value has synchronized to the ULP domain */
    while (WDT.STATUS & WDT_SYNCBUSY_bm)
    {
        /* spin — takes a few ULP cycles (~few µs at 4 MHz CPU) */
    }
}

/* ------------------------------------------------------------------ */
/*  LED initialisation                                                  */
/* ------------------------------------------------------------------ */
static void led_init(void)
{
    LED_PORT.DIRSET = LED_PIN_bm;   /* PC6 ? output */
    led_off();                       /* start with LED off */
}

/* ------------------------------------------------------------------ */
/*  Startup indicator pattern                                           */
/*  Lets you see on the hardware whether the MCU woke from POR or WDT. */
/*  The WDT is kicked between every blink so we don't accidentally      */
/*  reset during the indicator sequence itself.                         */
/* ------------------------------------------------------------------ */
static void show_reset_cause(uint8_t rstfr)
{
    uint8_t count = 1u;          /* default: 1 blink = power-on          */

    if (rstfr & RSTCTRL_WDRF_bm)
    {
        count = 3u;              /* 3 rapid blinks = WDT reset            */
    }
    else if (rstfr & RSTCTRL_SWRF_bm)
    {
        count = 2u;              /* 2 blinks = software reset             */
    }
    /* else PORF or EXTRF: 1 blink                                        */

    for (uint8_t i = 0u; i < count; i++)
    {
        wdt_kick();
        led_on();
        _delay_ms(INDICATOR_ON_MS);
        led_off();
        _delay_ms(INDICATOR_OFF_MS);
    }

    /* Pause after indicator so it is visually distinct from normal blinks */
    wdt_kick();
    _delay_ms(600u);
    wdt_kick();
}

/* ------------------------------------------------------------------ */
/*  Simulated firmware crash                                            */
/*  Stops kicking the WDT ? WDT fires after ~WDT_TIMEOUT.              */
/*  LED is left ON so the "frozen" state is obvious.                    */
/* ------------------------------------------------------------------ */
static void simulate_crash(void)
{
    led_on();                    /* solid LED = crashed firmware          */

    /*
     * Infinite loop — deliberately does NOT call wdt_kick().
     * After ~4.1 s the WDT fires and resets the MCU.
     * The volatile keyword prevents the compiler from optimising the
     * loop away.
     */
    while (1)
    {
        /* Do nothing — waiting for the WDT to bite */
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    /*
     * Step 1: Read and then clear the reset-cause flags BEFORE anything
     * else modifies them.  Clearing is done by writing 1s to the flags
     * (W1C — Write 1 to Clear), ensuring a clean slate for the next reset.
     */
    uint8_t reset_flags = RSTCTRL.RSTFR;
    RSTCTRL.RSTFR = reset_flags;    /* clear all flags (W1C) */

    /* Step 2: Initialise peripherals */
    led_init();

    /*
     * Step 3: Enable the watchdog.
     * From this point forward we MUST kick the WDT at least once every
     * ~4.1 s or the MCU will reset.
     */
    wdt_init();

    /* Step 4: Show how we got here (POR vs WDT reset) */
    show_reset_cause(reset_flags);

    /*
     * Step 5: Main loop — blink the LED and count toggles.
     * Each iteration: kick the WDT, toggle the LED, wait, repeat.
     * After CRASH_AFTER_TOGGLES cycles, drop into simulate_crash().
     */
    uint8_t toggle_count = 0u;

    while (1)
    {
        /* Kick the WDT at the top of every loop iteration */
        wdt_kick();

        /* Toggle the LED to show normal operation */
        led_toggle();

        /* Wait while the LED is in its current state */
        if (LED_PORT.IN & LED_PIN_bm)
        {
            /* LED is currently OFF */
            _delay_ms(BLINK_OFF_MS);
        }
        else
        {
            /* LED is currently ON */
            _delay_ms(BLINK_ON_MS);
        }

        toggle_count++;

        /* After enough toggles, simulate a crash (stop feeding the WDT) */
        if (toggle_count >= CRASH_AFTER_TOGGLES)
        {
            simulate_crash();   /* never returns — WDT resets the MCU */
        }
    }

    /* Unreachable */
    return 0;
}