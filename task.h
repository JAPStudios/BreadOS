#ifndef BREADOS_TASK_H
#define BREADOS_TASK_H

#include <stdint.h>

/* Initialize the round-robin kernel-thread scheduler before enabling IRQs. */
void tasking_init(void);

/* Called by the timer interrupt with its saved register frame. */
uint32_t *task_schedule(uint32_t *frame);

/* Number of task switches since initialization. */
uint32_t task_switch_count(void);

#endif
