#ifndef TCB_TIMER_H
#define TCB_TIMER_H

/*
 * TCB0 configured in Periodic Interrupt Mode.
 *
 * Clock source: CLK_PER (default 4 MHz after reset on AVR128DA48)
 * Prescaler:    DIV2  -> 2 MHz timer clock
 * CCMP:         1999  -> interrupt every 2000 counts = 1 ms
 *
 * If F_CPU is overridden to 24 MHz via FUSE / CLKCTRL, adjust CCMP:
 *   CCMP = (F_CPU / prescaler / 1000) - 1
 *
 * With default 4 MHz:  (4000000 / 2 / 1000) - 1 = 1999  <-- used here
 */

#define TCB0_CCMP_VALUE     1999u

void tcb0_init(void);

#endif /* TCB_TIMER_H */
