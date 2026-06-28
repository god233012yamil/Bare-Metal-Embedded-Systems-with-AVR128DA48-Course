#ifndef STORAGE_EEPROM_H_
#define STORAGE_EEPROM_H_

#include <stdint.h>

#define STORAGE_EEPROM_SIZE_BYTES 512u
#define STORAGE_EEPROM_BASE_ADDRESS 0u

void storage_eeprom_init(void);
void storage_eeprom_erase_region(void);
uint32_t sys_write(uint32_t address, void *ptr, uint32_t length);
uint32_t sys_read(uint32_t address, void *ptr, uint32_t length);

#endif
