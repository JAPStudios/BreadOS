#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H
#include <stdint.h>

/* Installs a kernel GDT and a 32-bit TSS. */
void gdt_init(void);

/* Kernel stack top used by the CPU when entering from ring 3. */
void gdt_set_kernel_stack(uint32_t stack_pointer);

/* Return the private ring-3 code selector assigned to a user task slot. */
uint16_t gdt_user_code_selector(uint32_t task_id);

/* Configure that task's private code descriptor to its executable ELF range. */
void gdt_set_user_code_limit(uint32_t task_id,uint32_t inclusive_limit);

#endif
