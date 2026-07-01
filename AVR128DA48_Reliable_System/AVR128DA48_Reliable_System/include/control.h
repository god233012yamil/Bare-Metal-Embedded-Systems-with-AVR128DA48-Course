#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdint.h>

void control_init(void);
void control_task(void);
void outputs_set_safe_state(void);
uint8_t control_get_output_level(void);

#endif
