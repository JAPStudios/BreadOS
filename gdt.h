#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H
#include <stdint.h>

/* Installs a kernel GDT and a 32-bit TSS. */
void gdt_init(void);

/* Kernel stack top used by the CPU when entering from ring 3. */
void gdt_set_kernel_stack(uint32_t stack_pointer);

#endif
