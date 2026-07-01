#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include "board.h"

/**
 * Initializes the AVR128DA48 clock, GPIO, and global interrupts.
 */
void board_init(void)
{
    ccp_write_io((void *)&CLKCTRL.MCLKCTRLB, 0x00);

    PORTA.DIRSET = BOARD_LED0_PIN_bm | BOARD_LED1_PIN_bm;
    PORTA.OUTCLR = BOARD_LED0_PIN_bm | BOARD_LED1_PIN_bm;

    PORTA.DIRCLR = BOARD_BUTTON_PIN_bm;
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;

    sei();
}

/**
 * Reads the active-low user button connected to PA2.
 *
 * Returns:
 *     1 when the button is pressed, otherwise 0.
 */
uint8_t board_read_button(void)
{
    return ((PORTA.IN & BOARD_BUTTON_PIN_bm) == 0U) ? 1U : 0U;
}

/**
 * Writes the LED0 output connected to PA6.
 *
 * Args:
 *     state: Non-zero turns the LED on. Zero turns the LED off.
 */
void board_led0_write(uint8_t state)
{
    if (state != 0U)
    {
        PORTA.OUTSET = BOARD_LED0_PIN_bm;
    }
    else
    {
        PORTA.OUTCLR = BOARD_LED0_PIN_bm;
    }
}

/**
 * Writes the LED1 output connected to PA7.
 *
 * Args:
 *     state: Non-zero turns the LED on. Zero turns the LED off.
 */
void board_led1_write(uint8_t state)
{
    if (state != 0U)
    {
        PORTA.OUTSET = BOARD_LED1_PIN_bm;
    }
    else
    {
        PORTA.OUTCLR = BOARD_LED1_PIN_bm;
    }
}

/**
 * Toggles LED0 connected to PA6.
 */
void board_led0_toggle(void)
{
    PORTA.OUTTGL = BOARD_LED0_PIN_bm;
}

/**
 * Toggles LED1 connected to PA7.
 */
void board_led1_toggle(void)
{
    PORTA.OUTTGL = BOARD_LED1_PIN_bm;
}
