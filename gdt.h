#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H
#include <stdint.h>

/* Installs a kernel GDT and a 32-bit TSS. */
void gdt_init(void);

/* Kernel stack top used by the CPU when entering from ring 3. */
void gdt_set_kernel_stack(uint32_t stack_pointer);

/* Restrict ring-3 instruction fetches to the executable ELF address range. */
void gdt_set_user_code_limit(uint32_t inclusive_limit);

#endif
