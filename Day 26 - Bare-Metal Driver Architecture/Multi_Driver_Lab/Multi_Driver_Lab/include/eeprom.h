/*
 * eeprom.h  --  Internal EEPROM driver public API, AVR128DA48
 *
 * The AVR128DA48 has 512 bytes of EEPROM mapped at data-space address
 * 0x1400 (EEPROM_START).  Access uses the NVMCTRL peripheral.
 *
 * Write strategy: EEPROM Erase-and-Write (NVMCTRL_CMD_EEERWR_gc) —
 * the safest single command that handles both erasure and programming
 * atomically per byte.
 *
 * CCP protection: NVMCTRL.CTRLA requires the CCP unlock sequence
 * (CCP = CCP_SPM_gc) immediately before writing the command.
 *
 * Busy polling: NVMCTRL.STATUS bit NVMCTRL_EEBUSY_bm is checked after
 * every write to ensure the previous operation is complete before the
 * next one begins.
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

/* AVR128DA48: 512 bytes of EEPROM */
#define EEPROM_SIZE_BYTES   512U
#define EEPROM_LAST_ADDR    (EEPROM_SIZE_BYTES - 1U)

/*
 * Public API
 */

/**
 * @brief  Read a single byte from EEPROM.
 * @param  addr  Byte offset (0 – 511).
 * @return Value stored at that address.
 */
uint8_t  EEPROM_ReadByte(uint16_t addr);

/**
 * @brief  Read multiple bytes from EEPROM into a RAM buffer.
 * @param  addr  Start byte offset.
 * @param  buf   Destination buffer.
 * @param  len   Number of bytes to read.
 */
void     EEPROM_ReadBlock(uint16_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief  Write a single byte to EEPROM (erase + write).
 *         Blocks until the write completes.
 * @param  addr  Byte offset (0 – 511).
 * @param  data  Value to write.
 */
void     EEPROM_WriteByte(uint16_t addr, uint8_t data);

/**
 * @brief  Write multiple bytes to EEPROM.
 *         Each byte is written individually with a busy-wait.
 * @param  addr  Start byte offset.
 * @param  buf   Source buffer.
 * @param  len   Number of bytes to write.
 */
void     EEPROM_WriteBlock(uint16_t addr, const uint8_t *buf, uint16_t len);

/**
 * @brief  Erase a single byte (set to 0xFF) using NVMCTRL_CMD_EEBER_gc.
 *         Blocks until the erase completes.
 * @param  addr  Byte offset (0 – 511).
 */
void     EEPROM_EraseByte(uint16_t addr);

/**
 * @brief  Erase the entire EEPROM (all bytes set to 0xFF).
 *         Uses NVMCTRL_CMD_EECHER_gc via CCP-protected write.
 *         Blocks until complete.
 */
void     EEPROM_EraseAll(void);

/**
 * @brief  Return non-zero if the EEPROM is currently busy with a write
 *         or erase operation (NVMCTRL_EEBUSY_bm in NVMCTRL.STATUS).
 */
uint8_t  EEPROM_IsBusy(void);

/**
 * @brief  Block until any pending EEPROM operation finishes.
 */
void     EEPROM_WaitReady(void);

#endif /* EEPROM_H_ */
