/*
 * uart.h  --  USART driver public API, AVR128DA48
 *
 * Covers USART0 – USART4 in asynchronous 8-N-1 mode.
 *
 * Default pin mapping (PORTMUX.USARTROUTEA):
 *   USART0: TX=PA0  RX=PA1
 *   USART1: TX=PC0  RX=PC1   <-- Curiosity Nano virtual COM port
 *   USART2: TX=PF0  RX=PF1
 *   USART3: TX=PB0  RX=PB1
 *   USART4: TX=PE0  RX=PE1
 *
 * The TX pin must be configured as an output by the caller before UART_Init().
 *
 * BAUD register formula (normal async, S=16):
 *   BAUD_REG = (64 * F_CPU) / (16 * baud) = (4 * F_CPU) / baud
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

/* USART instance selector */
typedef enum {
    UART_INSTANCE_0 = 0,  /* USART0 at 0x0800 */
    UART_INSTANCE_1,      /* USART1 at 0x0820  <-- Curiosity Nano CDC */
    UART_INSTANCE_2,      /* USART2 at 0x0840 */
    UART_INSTANCE_3,      /* USART3 at 0x0860 */
    UART_INSTANCE_4,      /* USART4 at 0x0880 */
} uart_instance_t;

/* Driver configuration */
typedef struct {
    uart_instance_t instance;
    uint32_t        baud;       /* Baud rate, e.g. 9600 */
    uint32_t        f_cpu_hz;   /* CPU frequency in Hz   */
} uart_config_t;

/*
 * Public API
 */

/**
 * @brief  Initialise a USART in 8-N-1 asynchronous mode (TX only).
 * @param  cfg  Pointer to a filled uart_config_t.
 */
void    UART_Init(const uart_config_t *cfg);

/**
 * @brief  Transmit a single byte (blocking, polls USART_DREIF_bm).
 * @param  inst  UART instance.
 * @param  byte  Byte to send.
 */
void    UART_SendByte(uart_instance_t inst, uint8_t byte);

/**
 * @brief  Transmit a null-terminated string.
 * @param  inst  UART instance.
 * @param  str   Pointer to string.
 */
void    UART_SendStr(uart_instance_t inst, const char *str);

/**
 * @brief  Transmit a 16-bit unsigned integer as decimal ASCII.
 * @param  inst  UART instance.
 * @param  val   Value to print.
 */
void    UART_SendU16(uart_instance_t inst, uint16_t val);

/**
 * @brief  Transmit a 32-bit unsigned integer as decimal ASCII.
 * @param  inst  UART instance.
 * @param  val   Value to print.
 */
void    UART_SendU32(uart_instance_t inst, uint32_t val);

/**
 * @brief  Transmit a byte as two uppercase hex digits.
 * @param  inst  UART instance.
 * @param  val   Byte to print.
 */
void    UART_SendHex8(uart_instance_t inst, uint8_t val);

#endif /* UART_H_ */
