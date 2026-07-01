#ifndef DIAGNOSTICS_H_
#define DIAGNOSTICS_H_

#include <stdint.h>

void diagnostics_init(void);
void diagnostics_task(void);
void diagnostics_print_startup_report(void);
void diagnostics_print_fault_report(void);
uint16_t diagnostics_get_stack_unused_bytes(void);

#endif
