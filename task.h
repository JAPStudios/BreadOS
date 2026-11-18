#ifndef BREADOS_TASK_H
#define BREADOS_TASK_H

#include <stdint.h>
#include "paging.h"

#define TASK_MAX 8u
#define TASK_KERNEL_COUNT 3u

enum task_state {
    TASK_UNUSED = 0,
    TASK_RUNNABLE = 1,
    TASK_ZOMBIE = 2
};

void tasking_init(void);
int task_set_kernel_stack(uint32_t stack_top);
int task_set_address_space(const struct paging_address_space *space);

/* Register a ring-3 task with its initial interrupt frame and private resources. */
int task_create_user(uint32_t *initial_frame,
                     const struct paging_address_space *space,
                     uint32_t kernel_stack_top,
                     uint32_t user_code_limit);

/* Mark the current user task for deferred cleanup after another address space is active. */
int task_terminate_current(int exit_code);
int task_is_zombie(uint32_t task_id);
int task_is_runnable(uint32_t task_id);
const struct paging_address_space *task_current_address_space(void);
int task_exit_code(uint32_t task_id);
int task_destroy(uint32_t task_id);
uint32_t task_current_id(void);
uint32_t *task_schedule(uint32_t *frame);
uint32_t task_switch_count(void);

#endif
