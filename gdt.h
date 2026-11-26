#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H
#include <stdint.h>
void gdt_init(void);
void gdt_set_kernel_stack(uint32_t stack_pointer);
#endif
