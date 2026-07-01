#ifndef BSP_H
#define BSP_H

#include <stdint.h>

#define F_CPU 4000000UL

void bsp_init(void);
void bsp_delay_ms(uint16_t delay_ms);

#endif
