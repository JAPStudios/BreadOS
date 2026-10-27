#ifndef BREADOS_INTERRUPTS_H
#define BREADOS_INTERRUPTS_H
#include <stdint.h>
void interrupts_init(void);
uint32_t timer_ticks(void);
#endif
