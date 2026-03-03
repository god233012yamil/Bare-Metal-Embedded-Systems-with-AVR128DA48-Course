/*
 * usart0.h
 *
 * USART0 Driver Header File for AVR128DA48
 * Polling-based implementation
 *
 * Created with Microchip Studio 7
 */

#ifndef USART0_H_
#define USART0_H_

#include <avr/io.h>
#include <stdint.h>

// Function prototypes

/**
 * @brief Initialize USART0 for asynchronous communication
 * @param None
 * @return None
 */
void USART0_init(void);

/**
 * @brief Transmit a single character via USART0 (polling method)
 * @param data The character/byte to transmit
 * @return None
 */
void USART0_sendChar(char data);

/**
 * @brief Transmit a null-terminated string via USART0 (polling method)
 * @param str Pointer to the null-terminated string to transmit
 * @return None
 */
void USART0_sendString(const char *str);

/**
 * @brief Receive a single character via USART0 (polling method)
 * @param None
 * @return The received character/byte
 */
char USART0_receiveChar(void);

/**
 * @brief Check if data is available in the USART0 receive buffer
 * @param None
 * @return 1 if data is available, 0 otherwise
 */
uint8_t USART0_isDataAvailable(void);

#endif /* USART0_H_ */