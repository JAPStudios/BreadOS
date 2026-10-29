#ifndef BREADOS_TASK_H
#define BREADOS_TASK_H

#include <stdint.h>

/* Initializes the round-robin kernel-thread scheduler. Call before enabling IRQs. */
void tasking_init(void);

/* Called from the timer interrupt with the saved interrupt frame. */
uint32_t *task_schedule(uint32_t *frame);

/* Number of timer-driven task switches since initialization. */
uint32_t task_switch_count(void);

#endif
