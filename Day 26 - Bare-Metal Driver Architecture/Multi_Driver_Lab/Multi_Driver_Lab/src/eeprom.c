/*
 * eeprom.c  --  Internal EEPROM driver implementation, AVR128DA48
 *
 * Peripheral : NVMCTRL (0x1000)
 * EEPROM     : data-space 0x1400 – 0x15FF  (EEPROM_START, 512 bytes)
 *
 * All register symbols from ioavr128da48.h via <avr/io.h>.
 *
 * Key symbols used
 *   NVMCTRL.CTRLA          -- command register (CCP-protected)
 *   NVMCTRL.STATUS         -- NVMCTRL_EEBUSY_bm, NVMCTRL_FBUSY_bm
 *   NVMCTRL_CMD_EEERWR_gc  -- Erase-and-Write byte
 *   NVMCTRL_CMD_EEBER_gc   -- Erase byte
 *   NVMCTRL_CMD_EECHER_gc  -- Erase entire EEPROM
 *   NVMCTRL_CMD_NONE_gc    -- clear command
 *   CCP_SPM_gc             -- CCP unlock value for SPM/NVM operations
 *   EEPROM_START           -- base address of EEPROM in data space (0x1400)
 *
 * The AVR-Dx EEPROM is memory-mapped: a simple pointer write into the
 * mapped EEPROM address space loads the page buffer, and then
 * CCP + NVMCTRL.CTRLA = command executes the NVM operation.
 */

#include "eeprom.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* Pointer to the EEPROM memory-mapped base in data space */
#define EEPROM_BASE_PTR   ((volatile uint8_t *)(EEPROM_START))

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static void prv_wait_nvm_ready(void)
{
    /* Wait for both flash and EEPROM operations to clear */
    while (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm | NVMCTRL_FBUSY_bm)) {}
}

/*
 * Issue a CCP-protected NVM command.
 * Interrupts must be disabled across the two-instruction CCP window.
 */
static void prv_execute_cmd(uint8_t cmd)
{
    uint8_t sreg = SREG;
    cli();
    CCP = CCP_SPM_gc;
    NVMCTRL.CTRLA = cmd;
    SREG = sreg;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

uint8_t EEPROM_IsBusy(void)
{
    return (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm) ? 1U : 0U;
}

void EEPROM_WaitReady(void)
{
    prv_wait_nvm_ready();
}

uint8_t EEPROM_ReadByte(uint16_t addr)
{
    prv_wait_nvm_ready();
    return EEPROM_BASE_PTR[addr];
}

void EEPROM_ReadBlock(uint16_t addr, uint8_t *buf, uint16_t len)
{
    prv_wait_nvm_ready();
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = EEPROM_BASE_PTR[addr + i];
    }
}

void EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
    prv_wait_nvm_ready();

    /* Write the byte into the memory-mapped EEPROM address to load the
     * page buffer, then issue the Erase-and-Write command. */
    EEPROM_BASE_PTR[addr] = data;
    prv_execute_cmd(NVMCTRL_CMD_EEERWR_gc);

    prv_wait_nvm_ready();

    /* Clear the command register */
    prv_execute_cmd(NVMCTRL_CMD_NONE_gc);
}

void EEPROM_WriteBlock(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        EEPROM_WriteByte((uint16_t)(addr + i), buf[i]);
    }
}

void EEPROM_EraseByte(uint16_t addr)
{
    prv_wait_nvm_ready();

    /* Write any value to the target address to select it, then erase */
    EEPROM_BASE_PTR[addr] = 0xFF;
    prv_execute_cmd(NVMCTRL_CMD_EEBER_gc);

    prv_wait_nvm_ready();
    prv_execute_cmd(NVMCTRL_CMD_NONE_gc);
}

void EEPROM_EraseAll(void)
{
    prv_wait_nvm_ready();
    prv_execute_cmd(NVMCTRL_CMD_EECHER_gc);
    prv_wait_nvm_ready();
    prv_execute_cmd(NVMCTRL_CMD_NONE_gc);
}
