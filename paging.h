#ifndef BREADOS_PAGING_H
#define BREADOS_PAGING_H
#include <stdint.h>

/* Enables supervisor-only, writable identity mapping of the first 16 MiB. */
void paging_init(void);

/* Maps a physical framebuffer into a dedicated 4 MiB virtual window. */
void *paging_map_framebuffer(uint64_t physical_address, uint32_t length);

/* Single-address-space user mapping window: 0x00400000..0x007FFFFF. */
int paging_map_user_page(uint32_t virtual_address, uint32_t physical_address,
                         int writable);
void paging_unmap_user_page(uint32_t virtual_address);

#endif
