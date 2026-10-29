#ifndef BREADOS_INTERRUPTS_H
#define BREADOS_INTERRUPTS_H
#include <stdint.h>
void interrupts_init(void);
uint32_t timer_ticks(void);
uint32_t *interrupt_handler(uint32_t vector, uint32_t error_code, uint32_t *frame);
#endif
