#include <avr/io.h>
#include <avr/cpufunc.h>
#include "reset_manager.h"

/**
 * @brief Captures and clears the MCU reset reason flags.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Raw RSTCTRL.RSTFR value captured before clearing the flags.
 */
uint8_t reset_manager_capture_reason(void)
{
    uint8_t reason = RSTCTRL.RSTFR;

    RSTCTRL.RSTFR = reason;

    return reason;
}

/**
 * @brief Forces a controlled software reset.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void reset_manager_software_reset(void)
{
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRF_bm); 

    while (1)
    {
        /* Wait for the reset controller to restart the MCU. */
    }
}
