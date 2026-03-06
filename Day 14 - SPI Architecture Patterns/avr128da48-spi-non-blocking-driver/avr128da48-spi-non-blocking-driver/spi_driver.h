/**
 * @file spi_nb.h
 * @brief Non-blocking SPI0 master driver (state machine + ISR) for AVR128DA48.
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SPI_STATE_IDLE = 0,
    SPI_STATE_ASSERT_CS,
    SPI_STATE_TRANSFER,
    SPI_STATE_COMPLETE,
    SPI_STATE_ERROR
} spi_state_t;

typedef enum
{
    SPI_NB_OK = 0,
    SPI_NB_ERR_BAD_PARAM = 1,
    SPI_NB_ERR_BUSY = 2,
    SPI_NB_ERR_TIMEOUT = 3,
    SPI_NB_ERR_INTERNAL = 4
} spi_nb_status_t;

spi_nb_status_t spi0_nb_init(void);

spi_nb_status_t spi0_nb_start(spi_device_id_t dev,
                              const uint8_t *tx_buf,
                              uint8_t *rx_buf,
                              uint16_t len,
                              uint32_t timeout_ms,
                              uint32_t now_ms);

void spi0_nb_task(uint32_t now_ms);

spi_state_t spi0_nb_get_state(void);

spi_nb_status_t spi0_nb_get_status(void);

void spi0_nb_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* SPI_DRIVER_H */
