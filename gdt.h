#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H
#include <stdint.h>

/* Installs a kernel GDT and a 32-bit TSS. */
void gdt_init(void);

/* Kernel stack top used by the CPU when entering from ring 3. */
void gdt_set_kernel_stack(uint32_t stack_pointer);

/* Set a process-private LDT code-segment limit before the task becomes runnable. */
void gdt_set_user_code_limit(uint32_t task_id,uint32_t inclusive_limit);

/* Load a process's private LDT; task IDs below TASK_KERNEL_COUNT clear LDTR. */
void gdt_load_task_ldt(uint32_t task_id);

#endif
