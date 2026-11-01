#ifndef BREADOS_PAGING_H
#define BREADOS_PAGING_H
#include <stdint.h>

/* Initial supervisor identity map: first 16 MiB only. */
void paging_init(void);

/* Maps a physical framebuffer into a dedicated 4 MiB virtual window.
 * Returns NULL if the address/range cannot be represented safely. */
void *paging_map_framebuffer(uint64_t physical_address, uint32_t length);

#endif
