#ifndef BREADOS_TASK_H
#define BREADOS_TASK_H

#include <stdint.h>
#include "paging.h"

/* Initialize the round-robin kernel-thread scheduler before enabling IRQs. */
void tasking_init(void);

/* Set the current task's ring-0 stack top for privilege transitions. */
int task_set_kernel_stack(uint32_t stack_top);

/* Associate the current kernel task with a private address space, or NULL for kernel-only. */
int task_set_address_space(const struct paging_address_space *space);

/* Called by the timer interrupt with its saved register frame. */
uint32_t *task_schedule(uint32_t *frame);

/* Number of task switches since initialization. */
uint32_t task_switch_count(void);

#endif
