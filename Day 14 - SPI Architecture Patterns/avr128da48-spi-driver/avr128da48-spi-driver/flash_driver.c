/**
 * @file    flash_driver.c
 * @brief   SPI NOR-Flash driver implementation (W25Q32JV / W25Q128JV).
 *
 * Demonstrates how to build a device-specific driver on top of the reusable
 * SPI layer defined in spi_driver.h.  Key patterns shown:
 *
 *   1. Translating device commands into SPI_TransferBuffer() calls.
 *   2. Polling the device status register with a timeout guard.
 *   3. Enforcing alignment and range checks before issuing destructive
 *      operations (erase, write).
 *   4. Keeping CS assertion / de-assertion explicit at the flash-driver
 *      level so the SPI driver remains CS-agnostic.
 *
 * @author  Your Name
 * @date    2026-02-17
 */

#include "flash_driver.h"
#include <util/delay.h>

/* =========================================================================
 * Private constants – W25Q series command set
 * ========================================================================= */

/** Read JEDEC ID: returns Manufacturer ID, Memory Type, Capacity.          */
#define FLASH_CMD_READ_ID         0x9FU

/** Fast Read: opcode + 3-byte address + 1 dummy byte, then data.           */
#define FLASH_CMD_FAST_READ       0x0BU

/** Write Enable: must precede every erase or program operation.            */
#define FLASH_CMD_WRITE_ENABLE    0x06U

/** Sector Erase (4 KB): opcode + 3-byte sector address.                   */
#define FLASH_CMD_SECTOR_ERASE    0x20U

/** Page Program (up to 256 bytes): opcode + 3-byte address + data.         */
#define FLASH_CMD_PAGE_PROGRAM    0x02U

/** Read Status Register-1: bit 0 is BUSY.                                  */
#define FLASH_CMD_READ_STATUS1    0x05U

/** Mask for the BUSY flag in Status Register 1.                             */
#define FLASH_SR1_BUSY_bm         0x01U

/* =========================================================================
 * Private state
 * ========================================================================= */

/** PORT and pin used as chip select – set once in Flash_Init().            */
static PORT_t  *s_csPort  = NULL;
static uint8_t  s_csPinBm = 0x00U;

/* =========================================================================
 * Private helper macros / inline functions
 * ========================================================================= */

/** Assert the flash chip select.                                            */
#define FLASH_CS_ASSERT()   SPI_CS_Low(s_csPort,  s_csPinBm)

/** De-assert the flash chip select.                                         */
#define FLASH_CS_RELEASE()  SPI_CS_High(s_csPort, s_csPinBm)

/**
 * @brief  Map an SPI_Status_t error to the nearest Flash_Status_t.
 * @param  s  SPI status value.
 * @return    Corresponding Flash_Status_t.
 */
static inline Flash_Status_t spi_to_flash_status(SPI_Status_t s)
{
    if (s == SPI_OK)          return FLASH_OK;
    if (s == SPI_ERR_TIMEOUT) return FLASH_ERR_TIMEOUT;
    return FLASH_ERR_SPI;
}

/* =========================================================================
 * Public API implementation
 * ========================================================================= */

Flash_Status_t Flash_Init(PORT_t *csPort, uint8_t csPinBm)
{
    /* Store CS configuration for use by all subsequent operations.         */
    s_csPort  = csPort;
    s_csPinBm = csPinBm;

    /* Configure the CS pin as output, de-asserted (high).                  */
    csPort->OUTSET = csPinBm;
    csPort->DIRSET = csPinBm;

    /* Initialise the SPI peripheral.  Mode 0, 1 MHz (DIV4 @ 4 MHz).       */
    SPI_Init(FLASH_SPI_MODE, SPI_PRESCALER_DIV4, false);

    /* Verify device presence by reading the JEDEC ID.                      */
    Flash_ID_t id;
    Flash_Status_t st = Flash_ReadID(&id);
    if (st != FLASH_OK)
    {
        return st;
    }

    /* Sanity-check: Winbond devices always return 0xEF.
     * Modify this check for other flash manufacturers.                     */
    if (id.manufacturer_id != 0xEFU)
    {
        return FLASH_ERR_SPI;   /* unexpected ID – device may be absent     */
    }

    return FLASH_OK;
}

/* -------------------------------------------------------------------------- */

Flash_Status_t Flash_ReadID(Flash_ID_t *id)
{
    if (id == NULL)
    {
        return FLASH_ERR_ARG;
    }

    /* JEDEC Read-ID: 1 command byte + 3 response bytes.
     * TX buffer contains the opcode followed by dummy bytes.               */
    const uint8_t txBuf[4] = { FLASH_CMD_READ_ID, 0x00U, 0x00U, 0x00U };
    uint8_t       rxBuf[4] = { 0 };

    FLASH_CS_ASSERT();
    SPI_Status_t spiSt = SPI_TransferBuffer(txBuf, rxBuf, sizeof(txBuf));
    FLASH_CS_RELEASE();

    if (spiSt != SPI_OK)
    {
        return spi_to_flash_status(spiSt);
    }

    /* rxBuf[0] mirrors the opcode (shift-register artifact – ignore it).
     * rxBuf[1..3] carry the JEDEC identification bytes.                    */
    id->manufacturer_id = rxBuf[1];
    id->memory_type     = rxBuf[2];
    id->capacity        = rxBuf[3];

    return FLASH_OK;
}

/* -------------------------------------------------------------------------- */

Flash_Status_t Flash_Read(uint32_t address, uint8_t *buf, size_t len)
{
    if ((buf == NULL) || (len == 0U))
    {
        return FLASH_ERR_ARG;
    }

    /* Fast Read command frame: [0x0B][A23][A15][A7][DummyByte] then data.
     * We build a 5-byte header and use SPI_TransferBuffer for the rest.    */
    uint8_t header[5] =
    {
        FLASH_CMD_FAST_READ,
        (uint8_t)((address >> 16U) & 0xFFU),  /* Address bits 23-16         */
        (uint8_t)((address >>  8U) & 0xFFU),  /* Address bits 15-8          */
        (uint8_t)((address)        & 0xFFU),  /* Address bits 7-0           */
        0x00U                                  /* Required dummy byte        */
    };

    FLASH_CS_ASSERT();

    /* Send the 5-byte header (TX only – discard received bytes).           */
    SPI_Status_t spiSt = SPI_TransferBuffer(header, NULL, sizeof(header));
    if (spiSt == SPI_OK)
    {
        /* Clock in the actual data bytes (send dummy 0xFF, receive data).  */
        spiSt = SPI_TransferBuffer(NULL, buf, len);
    }

    FLASH_CS_RELEASE();

    return spi_to_flash_status(spiSt);
}

/* -------------------------------------------------------------------------- */

Flash_Status_t Flash_SectorErase(uint32_t address)
{
    /* Align the address to the start of the containing 4 KB sector.       */
    uint32_t sectorAddr = address & ~(uint32_t)(FLASH_SECTOR_SIZE_BYTES - 1U);

    /* Issue Write Enable first – required before any erase or program op.  */
    uint8_t weCmd = FLASH_CMD_WRITE_ENABLE;
    FLASH_CS_ASSERT();
    SPI_Status_t spiSt = SPI_TransferBuffer(&weCmd, NULL, 1U);
    FLASH_CS_RELEASE();

    if (spiSt != SPI_OK)
    {
        return spi_to_flash_status(spiSt);
    }

    /* Small inter-command gap recommended by most flash data sheets.       */
    _delay_us(1);

    /* Issue the Sector Erase command with the 24-bit sector address.       */
    uint8_t eraseCmd[4] =
    {
        FLASH_CMD_SECTOR_ERASE,
        (uint8_t)((sectorAddr >> 16U) & 0xFFU),
        (uint8_t)((sectorAddr >>  8U) & 0xFFU),
        (uint8_t)((sectorAddr)        & 0xFFU)
    };

    FLASH_CS_ASSERT();
    spiSt = SPI_TransferBuffer(eraseCmd, NULL, sizeof(eraseCmd));
    FLASH_CS_RELEASE();

    if (spiSt != SPI_OK)
    {
        return spi_to_flash_status(spiSt);
    }

    /* Wait for erase to complete (typical 4 KB erase = 45–400 ms).        */
    return Flash_WaitReady(500U);
}

/* -------------------------------------------------------------------------- */

Flash_Status_t Flash_PageWrite(uint32_t       address,
                               const uint8_t *buf,
                               size_t         len)
{
    /* Enforce page alignment and length constraints.                        */
    if ((address % FLASH_PAGE_SIZE_BYTES) != 0U)
    {
        return FLASH_ERR_ALIGN;
    }
    if ((len == 0U) || (len > FLASH_PAGE_SIZE_BYTES))
    {
        return FLASH_ERR_ALIGN;
    }
    if (buf == NULL)
    {
        return FLASH_ERR_ARG;
    }

    /* Write Enable.                                                         */
    uint8_t weCmd = FLASH_CMD_WRITE_ENABLE;
    FLASH_CS_ASSERT();
    SPI_Status_t spiSt = SPI_TransferBuffer(&weCmd, NULL, 1U);
    FLASH_CS_RELEASE();

    if (spiSt != SPI_OK)
    {
        return spi_to_flash_status(spiSt);
    }

    _delay_us(1);

    /* Page Program: 4-byte header then up to 256 data bytes in one
     * continuous CS-low transaction.                                        */
    uint8_t pgCmd[4] =
    {
        FLASH_CMD_PAGE_PROGRAM,
        (uint8_t)((address >> 16U) & 0xFFU),
        (uint8_t)((address >>  8U) & 0xFFU),
        (uint8_t)((address)        & 0xFFU)
    };

    FLASH_CS_ASSERT();

    spiSt = SPI_TransferBuffer(pgCmd, NULL, sizeof(pgCmd));
    if (spiSt == SPI_OK)
    {
        /* Transmit the data payload in the same CS-low frame.              */
        spiSt = SPI_TransferBuffer(buf, NULL, len);
    }

    FLASH_CS_RELEASE();

    if (spiSt != SPI_OK)
    {
        return spi_to_flash_status(spiSt);
    }

    /* Wait for programming to complete (typical page program = 0.4–3 ms). */
    return Flash_WaitReady(10U);
}

/* -------------------------------------------------------------------------- */

Flash_Status_t Flash_WaitReady(uint16_t timeout_ms)
{
    /* Poll Status Register 1 bit 0 (BUSY) until clear.
     * We approximate one iteration as ~0.5 ms at 4 MHz (rough estimate;
     * tighten with a hardware timer if needed).                             */
    for (uint16_t i = 0U; i < (timeout_ms * 2U); i++)
    {
        const uint8_t txBuf[2] = { FLASH_CMD_READ_STATUS1, 0x00U };
        uint8_t       rxBuf[2] = { 0 };

        FLASH_CS_ASSERT();
        SPI_Status_t spiSt = SPI_TransferBuffer(txBuf, rxBuf, sizeof(txBuf));
        FLASH_CS_RELEASE();

        if (spiSt != SPI_OK)
        {
            return spi_to_flash_status(spiSt);
        }

        /* rxBuf[1] holds the status register value.                        */
        if (!(rxBuf[1] & FLASH_SR1_BUSY_bm))
        {
            return FLASH_OK;   /* device ready                              */
        }

        _delay_us(500);        /* wait ~0.5 ms before next poll             */
    }

    return FLASH_ERR_TIMEOUT;
}
