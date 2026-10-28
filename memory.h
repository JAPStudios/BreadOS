#ifndef BREADOS_MEMORY_H
#define BREADOS_MEMORY_H
#include <stdint.h>

/* Physical frames are 4 KiB. Returned addresses are physical addresses. */
void memory_init(uint32_t multiboot_info_address);
uint32_t memory_alloc_frame(void); /* returns 0 when exhausted */
void memory_free_frame(uint32_t physical_address);
uint32_t memory_free_frames(void);
uint32_t memory_total_frames(void);

#endif
