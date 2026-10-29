#ifndef BREADOS_GDT_H
#define BREADOS_GDT_H

/* Installs a kernel GDT and a 32-bit TSS. This is prerequisite infrastructure
 * for ring-3 processes; it does not itself provide process isolation. */
void gdt_init(void);

#endif
