#ifndef BREADOS_PAGING_H
#define BREADOS_PAGING_H

/* Enables supervisor-only, writable identity mapping of the first 16 MiB. */
void paging_init(void);

#endif
