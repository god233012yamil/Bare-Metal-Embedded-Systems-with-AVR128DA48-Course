#include <avr/io.h>
#include <avr/eeprom.h>
#include "fault.h"
#include "system_time.h"
#include "config.h"

#define FAULT_RECORD_MAGIC       0xFA170048UL
#define FAULT_CRC_SEED           0xFFFFU

static fault_record_t g_fault_record;
static fault_record_t EEMEM g_eeprom_fault_record;

static uint16_t fault_crc16(const uint8_t *data, uint16_t length);

/**
 * @brief Initializes the fault manager and updates boot counters.
 *
 * Args:
 *     reset_reason: Reset reason captured from RSTCTRL.RSTFR.
 *
 * Returns:
 *     None.
 */
void fault_init(uint8_t reset_reason)
{
    eeprom_read_block(&g_fault_record, &g_eeprom_fault_record, sizeof(g_fault_record));

    if (g_fault_record.magic != FAULT_RECORD_MAGIC)
    {
        g_fault_record.magic = FAULT_RECORD_MAGIC;
        g_fault_record.boot_count = 0;
        g_fault_record.failed_boot_count = 0;
        g_fault_record.fault_code = FAULT_NONE;
        g_fault_record.system_state = 0;
        g_fault_record.uptime_ms = 0;
    }

    g_fault_record.boot_count++;
    g_fault_record.reset_reason = reset_reason;

    if ((reset_reason & RSTCTRL_WDRF_bm) != 0U)
    {
        g_fault_record.failed_boot_count++;
        g_fault_record.fault_code = FAULT_WATCHDOG_RESET;
    }

    g_fault_record.crc = fault_crc16((const uint8_t *)&g_fault_record, sizeof(g_fault_record) - sizeof(g_fault_record.crc));
    eeprom_update_block(&g_fault_record, &g_eeprom_fault_record, sizeof(g_fault_record));
}

/**
 * @brief Stores the latest fault code in RAM and EEPROM.
 *
 * Args:
 *     fault: Fault code to store.
 *
 * Returns:
 *     None.
 */
void fault_report(fault_code_t fault)
{
    g_fault_record.fault_code = (uint8_t)fault;
    g_fault_record.uptime_ms = system_time_get_ms();
    g_fault_record.crc = fault_crc16((const uint8_t *)&g_fault_record, sizeof(g_fault_record) - sizeof(g_fault_record.crc));
    eeprom_update_block(&g_fault_record, &g_eeprom_fault_record, sizeof(g_fault_record));
}

/**
 * @brief Returns the last stored fault code.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Last fault code.
 */
fault_code_t fault_get_last(void)
{
    return (fault_code_t)g_fault_record.fault_code;
}

/**
 * @brief Returns the lifetime boot counter.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Number of recorded boots.
 */
uint32_t fault_get_boot_count(void)
{
    return g_fault_record.boot_count;
}

/**
 * @brief Returns the number of consecutive failed boots.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Consecutive failed boot count.
 */
uint32_t fault_get_failed_boot_count(void)
{
    return g_fault_record.failed_boot_count;
}

/**
 * @brief Marks the current boot as successful after startup is complete.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     None.
 */
void fault_mark_boot_successful(void)
{
    g_fault_record.failed_boot_count = 0;
    g_fault_record.crc = fault_crc16((const uint8_t *)&g_fault_record, sizeof(g_fault_record) - sizeof(g_fault_record.crc));
    eeprom_update_block(&g_fault_record, &g_eeprom_fault_record, sizeof(g_fault_record));
}

/**
 * @brief Saves a snapshot of the current firmware state.
 *
 * Args:
 *     system_state: Current system state value.
 *
 * Returns:
 *     None.
 */
void fault_save_snapshot(uint8_t system_state)
{
    g_fault_record.system_state = system_state;
    g_fault_record.uptime_ms = system_time_get_ms();
    g_fault_record.crc = fault_crc16((const uint8_t *)&g_fault_record, sizeof(g_fault_record) - sizeof(g_fault_record.crc));
    eeprom_update_block(&g_fault_record, &g_eeprom_fault_record, sizeof(g_fault_record));
}

/**
 * @brief Checks whether safe mode should be entered.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     true if repeated failed boots require safe mode, otherwise false.
 */
bool fault_safe_mode_required(void)
{
    return (g_fault_record.failed_boot_count >= MAX_FAILED_BOOTS);
}

/**
 * @brief Returns a pointer to the current fault record.
 *
 * Args:
 *     None.
 *
 * Returns:
 *     Pointer to the active fault record.
 */
const fault_record_t *fault_get_record(void)
{
    return &g_fault_record;
}

/**
 * @brief Calculates a CRC-16/CCITT-FALSE checksum.
 *
 * Args:
 *     data: Pointer to the byte buffer.
 *     length: Number of bytes to process.
 *
 * Returns:
 *     Calculated CRC value.
 */
static uint16_t fault_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = FAULT_CRC_SEED;

    if (data == 0)
    {
        return 0;
    }

    while (length > 0U)
    {
        crc ^= (uint16_t)(*data) << 8;

        for (uint8_t bit = 0; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc <<= 1;
            }
        }

        data++;
        length--;
    }

    return crc;
}
