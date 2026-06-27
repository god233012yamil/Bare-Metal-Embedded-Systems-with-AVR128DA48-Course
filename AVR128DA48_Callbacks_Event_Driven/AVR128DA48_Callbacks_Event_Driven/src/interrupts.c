#include "button.h"
#include "system_tick.h"

#include <avr/interrupt.h>
#include <avr/io.h>

/**
 * @brief Handles PORTC pin interrupts and forwards button events.
 */
ISR(PORTC_PORT_vect)
{
    const uint8_t interrupt_flags = PORTC.INTFLAGS;

    /* Clear captured flags by writing ones before invoking application code. */
    PORTC.INTFLAGS = interrupt_flags;

    if ((interrupt_flags & PIN7_bm) != 0U)
    {
        button_irq_handler();
    }
}

/**
 * @brief Handles the 1 ms TCB0 periodic interrupt.
 */
ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    system_tick_irq_handler();
}
