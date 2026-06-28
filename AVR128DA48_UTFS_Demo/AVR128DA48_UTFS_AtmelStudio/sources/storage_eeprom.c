#include "storage_eeprom.h"

#include <avr/eeprom.h>
#include <stdint.h>

/**
 * Initializes the EEPROM storage backend used by UTFS.
 *
 * The AVR EEPROM driver in avr-libc does not require peripheral setup, so this
 * function is intentionally small. It is kept as a porting seam for projects
 * that later move UTFS to SPI flash, I2C EEPROM, or FRAM.
 */
void storage_eeprom_init(void)
{
    /* No setup is required for avr-libc EEPROM access. */
}

/**
 * Erases the full UTFS EEPROM region to 0xFF.
 *
 * This is useful during development when the file layout changes or when the
 * user wants to force first-boot default values.
 */
void storage_eeprom_erase_region(void)
{
    for (uint16_t address = 0u; address < STORAGE_EEPROM_SIZE_BYTES; address++) {
        eeprom_update_byte((uint8_t *)address, 0xFFu);
    }
}

/**
 * Writes bytes to the EEPROM-backed UTFS medium.
 *
 * Args:
 *     address: Byte offset from the start of the EEPROM region.
 *     ptr: Source buffer containing the bytes to write.
 *     length: Number of bytes to write.
 *
 * Returns:
 *     Number of bytes written. A short write indicates that the request would
 *     exceed the configured EEPROM region.
 */
uint32_t sys_write(uint32_t address, void *ptr, uint32_t length)
{
    uint8_t *source = (uint8_t *)ptr;

    if ((ptr == 0) || (address >= STORAGE_EEPROM_SIZE_BYTES)) {
        return 0u;
    }

    if ((address + length) > STORAGE_EEPROM_SIZE_BYTES) {
        length = STORAGE_EEPROM_SIZE_BYTES - address;
    }

    for (uint32_t index = 0u; index < length; index++) {
        eeprom_update_byte((uint8_t *)(uintptr_t)(address + index), source[index]);
    }

    return length;
}

/**
 * Reads bytes from the EEPROM-backed UTFS medium.
 *
 * Args:
 *     address: Byte offset from the start of the EEPROM region.
 *     ptr: Destination buffer that receives the bytes.
 *     length: Number of bytes to read.
 *
 * Returns:
 *     Number of bytes read. A short read indicates that the request would exceed
 *     the configured EEPROM region.
 */
uint32_t sys_read(uint32_t address, void *ptr, uint32_t length)
{
    uint8_t *destination = (uint8_t *)ptr;

    if ((ptr == 0) || (address >= STORAGE_EEPROM_SIZE_BYTES)) {
        return 0u;
    }

    if ((address + length) > STORAGE_EEPROM_SIZE_BYTES) {
        length = STORAGE_EEPROM_SIZE_BYTES - address;
    }

    for (uint32_t index = 0u; index < length; index++) {
        destination[index] = eeprom_read_byte((const uint8_t *)(uintptr_t)(address + index));
    }

    return length;
}
