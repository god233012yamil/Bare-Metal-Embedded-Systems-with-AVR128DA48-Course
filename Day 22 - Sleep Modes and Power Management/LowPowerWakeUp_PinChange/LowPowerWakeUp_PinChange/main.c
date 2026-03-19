/**
 * @file    main.c
 * @brief   Low-Power Wake-Up via Pin Change Interrupt (SW0 / PC7)
 *
 * Target:    AVR128DA48 (Curiosity Nano)
 * Toolchain: Atmel/Microchip Studio 7 – GCC C Executable Project
 * Device Pack: AVR-Dx 2.4.286
 *
 * Description:
 *   The MCU sleeps in Power-Down mode indefinitely.
 *   Pressing the on-board button SW0 (PC7, active LOW) generates a
 *   falling-edge pin change interrupt that wakes the CPU.
 *   In the ISR the LED (PC6, active LOW) is toggled and the CPU
 *   returns to Power-Down sleep.
 *
 *   Pin change interrupts on AVR-Dx:
 *   ?????????????????????????????????????????????????????????????????????
 *   There is no dedicated "PCINT" peripheral on the AVR-Dx family.
 *   Instead, every GPIO pin has an Input/Sense Configuration (ISC) field
 *   in its PINnCTRL register that selects the interrupt trigger condition:
 *
 *     PORT_ISC_INTDISABLE_gc  – interrupt disabled, buffer enabled
 *     PORT_ISC_BOTHEDGES_gc   – interrupt on both edges
 *     PORT_ISC_RISING_gc      – interrupt on rising edge
 *     PORT_ISC_FALLING_gc     – interrupt on falling edge  <-- used here
 *     PORT_ISC_INPUT_DISABLE_gc – digital buffer off
 *     PORT_ISC_LEVEL_gc       – interrupt on low level
 *
 *   When the condition is met, the corresponding bit in PORT.INTFLAGS
 *   is set and the shared PORTx_PORT_vect ISR fires.
 *   The ISR must clear the flag by writing 1 to the set bit.
 *
 *   Hardware connections (Curiosity Nano):
 *   ?????????????????????????????????????????????????????????????????????
 *   SW0  ? PC7  (active LOW, internal pull-up enabled)
 *   LED0 ? PC6  (active LOW)
 *
 *   Configuration summary:
 *   ?????????????????????????????????????????????????????????????????????
 *   PORTC.PIN7CTRL = PORT_PULLUPEN_bm          — enable internal pull-up
 *                  | PORT_ISC_FALLING_gc;       — interrupt on falling edge
 *   Sleep mode     = Power-Down (SLPCTRL_SMODE_PDOWN_gc)
 *   ISR vector     = PORTC_PORT_vect
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* ------------------------------------------------------------------ */
/* Defines                                                             */
/* ------------------------------------------------------------------ */
#define LED_PIN     PIN6_bm     /* PC6 – LED0, active LOW              */
#define BTN_PIN     PIN7_bm     /* PC7 – SW0,  active LOW              */

/* ================================================================== */
/* Prototypes                                                          */
/* ================================================================== */
static void PORT_init(void);
static void SLEEP_init(void);

/* ================================================================== */
/* Interrupt Service Routine                                           */
/* ================================================================== */
/**
 * PORTC Pin Change Interrupt.
 *
 * Shared by all PC0–PC7 pins that have ISC != INTDISABLE.
 * Check PORTC.INTFLAGS to identify which pin triggered the interrupt,
 * clear the flag by writing 1 to it, then act.
 */
ISR(PORTC_PORT_vect)
{
    /* Check that it was PC7 (SW0) that triggered */
    if (PORTC.INTFLAGS & BTN_PIN)
    {
        /* Clear the interrupt flag (write 1 to clear) */
        PORTC.INTFLAGS = BTN_PIN;

        /* Toggle LED (PC6, active LOW) */
        PORTC.OUTTGL = LED_PIN;
    }
}

/* ================================================================== */
/* Peripheral initialisation functions                                 */
/* ================================================================== */

/**
 * @brief Configure PC6 (LED0) and PC7 (SW0) on PORTC.
 *
 * LED0 (PC6):
 *   - Output, driven HIGH initially (LED off, active LOW).
 *
 * SW0 (PC7):
 *   - Input with internal pull-up enabled.
 *   - Falling-edge ISC: interrupt fires when the button is pressed
 *     (pin pulled LOW by the switch).
 *   - The PORTC_PORT_vect ISR handles the wake-up.
 */
static void PORT_init(void)
{
    /* ?? LED0 on PC6 ?? */
    PORTC.DIRSET  = LED_PIN;            /* PC6 as output              */
    PORTC.OUTSET  = LED_PIN;            /* LED off initially          */

    /* ?? SW0 on PC7 ?? */
    PORTC.DIRCLR  = BTN_PIN;            /* PC7 as input               */

    /* Enable internal pull-up + falling-edge interrupt sense          */
    PORTC.PIN7CTRL = PORT_PULLUPEN_bm   /* Internal pull-up on PC7    */
                   | PORT_ISC_FALLING_gc; /* Interrupt on falling edge */
}

/**
 * @brief Select Power-Down as the sleep mode.
 *
 * Power-Down stops CLK_PER and most peripherals.
 * Pin change interrupts are detected asynchronously and can wake
 * the MCU from Power-Down without any peripheral clock running.
 */
static void SLEEP_init(void)
{
    SLPCTRL.CTRLA = SLPCTRL_SMODE_PDOWN_gc  /* Power-Down mode */
                  | SLPCTRL_SEN_bm;          /* Sleep enable    */
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */
int main(void)
{
	// Configure PC6 (LED0) and PC7 (SW0) on PORTC.
	PORT_init();
	
	// Select Power-Down as the sleep mode.
	SLEEP_init();

	// enable Global interrupt
	sei();          
	
	for (;;)
	{
		// Power-Down; wake on PC7 falling edge
		sleep_cpu(); 
	}
}