/*
 * AVR128DA48 - Read all fuses (bare-metal)
 *
 * Reads the 16 fuse bytes from the memory-mapped fuse region
 * starting at FUSES_START (0x1050).
 *
 * Output:
 * - g_fuses[] holds the raw fuse bytes (inspect in debugger).
 */

#include <avr/io.h>
#include <stdint.h>

static volatile uint8_t g_fuses[FUSES_SIZE];

/*
 * Read all fuse bytes into the provided buffer.
 *
 * buf must be at least FUSES_SIZE bytes.
 */
static void fuses_read_all(uint8_t *buf)
{
    const volatile uint8_t *fuse_ptr = (const volatile uint8_t *)FUSES_START;

    for (uint8_t i = 0; i < (uint8_t)FUSES_SIZE; i++)
    {
        buf[i] = fuse_ptr[i];
    }
}

int main(void)
{
    fuses_read_all((uint8_t *)g_fuses);

    while (1)
    {
        /* Put a breakpoint here and inspect g_fuses[] */
    }
}