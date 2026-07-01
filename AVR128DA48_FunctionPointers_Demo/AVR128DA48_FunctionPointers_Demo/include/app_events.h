#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include <stdint.h>
#include "button.h"

void app_init(void);
void app_process(void);
void app_button_event_handler(button_event_t event, void *context);
void app_uart_rx_handler(uint8_t data, void *context);
void app_request_tick(void);

#endif
