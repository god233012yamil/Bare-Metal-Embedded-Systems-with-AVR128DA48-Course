#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#define BOARD_LED0_PIN_bm PIN6_bm
#define BOARD_LED1_PIN_bm PIN7_bm
#define BOARD_BUTTON_PIN_bm PIN2_bm
#define BOARD_UART_BAUD_RATE 9600UL

void board_init(void);
uint8_t board_read_button(void);
void board_led0_write(uint8_t state);
void board_led1_write(uint8_t state);
void board_led0_toggle(void);
void board_led1_toggle(void);

#endif
