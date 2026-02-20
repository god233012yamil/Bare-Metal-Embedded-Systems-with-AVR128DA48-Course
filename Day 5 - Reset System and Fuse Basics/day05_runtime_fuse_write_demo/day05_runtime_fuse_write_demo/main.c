/*
 * AVR128DA48 - Runtime Fuse Write Demonstration
 *
 * WARNING:
 * - Fuse writes are permanent until reprogrammed
 * - Incorrect values can disable UPDI or clock sources
 * - This example is for educational purposes only
 *
 * This code demonstrates:
 * - How fuses are memory-mapped on AVR DA devices
 * - How NVMCTRL is used to write fuse bytes
 */

#include <avr/io.h>
#include <stdint.h>

/* -------------------------------------------------- */
/* CCP-protected register write helper                */
/* -------------------------------------------------- */
static inline void ccp_write_io(volatile uint8_t *addr, uint8_t value)
{
    CCP = CCP_IOREG_gc;
    *addr = value;
}

/* -------------------------------------------------- */
/* Fuse write helper (DA-series, header-correct)      */
/* -------------------------------------------------- */

/*
 * Write a single fuse byte.
 *
 * offset : byte offset into the fuse region (0..15)
 * value  : new fuse byte value
 */
static void fuse_write_byte(uint8_t offset, uint8_t value)
{
    uint32_t fuse_addr = (uint32_t)(FUSES_START + offset);

    /* Wait until Flash NVM is ready */
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm)
    {
        /* wait */
    }

    /* Select target address in fuse region */
    NVMCTRL.ADDR = fuse_addr;

    /* Load data (low byte only for single fuse byte) */
    NVMCTRL.DATAL = value;

    /* Execute Flash Write command */
    ccp_write_io(&NVMCTRL.CTRLA, (uint8_t)NVMCTRL_CMD_FLWR_gc);

    /* Wait for completion */
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm)
    {
        /* wait */
    }
}

/* -------------------------------------------------- */
/* Application entry point                            */
/* -------------------------------------------------- */

int main(void)
{
    /*
     * EXAMPLE ONLY
     *
     * The offset and value below are placeholders.
     * Always verify fuse offsets and meanings in the datasheet
     * and ioavr128da48.h before running code like this.
     */

    /* Example: write fuse byte at offset 0 (typically WDTCFG) */
    /* fuse_write_byte(0, 0x00); */

    while (1)
    {
        /* Device must be reset for fuse changes to take effect */
    }
}