#include "board.h"

#include <avr/io.h>

#define BOARD_LED_PIN PIN6_bm
#define BOARD_BUTTON_PIN PIN7_bm

/**
 * @brief Initializes the board GPIO used by the demonstration.
 */
void board_init(void)
{
    /* The AVR128DA48 Curiosity Nano LED on PC6 is active low. */
    PORTC.OUTSET = BOARD_LED_PIN;
    PORTC.DIRSET = BOARD_LED_PIN;

    /* Enable the internal pull-up before enabling falling-edge interrupts. */
    PORTC.DIRCLR = BOARD_BUTTON_PIN;
    PORTC.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}

/**
 * @brief Turns the user LED on.
 */
void board_led_on(void)
{
    PORTC.OUTCLR = BOARD_LED_PIN;
}

/**
 * @brief Turns the user LED off.
 */
void board_led_off(void)
{
    PORTC.OUTSET = BOARD_LED_PIN;
}

/**
 * @brief Toggles the current user LED state.
 */
void board_led_toggle(void)
{
    PORTC.OUTTGL = BOARD_LED_PIN;
}

/**
 * @brief Reads the user button input.
 *
 * Returns:
 *     true while the active-low button is pressed, otherwise false.
 */
bool board_button_is_pressed(void)
{
    return (PORTC.IN & BOARD_BUTTON_PIN) == 0U;
}
