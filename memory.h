#ifndef BREADOS_MEMORY_H
#define BREADOS_MEMORY_H
#include <stdint.h>

/* Physical frames are 4 KiB. The current bootstrap page table identity-maps
 * only the first 16 MiB, so allocation/free operations are limited to that
 * directly mapped region. Returned addresses are physical addresses.
 */
void memory_init(uint32_t multiboot_info_address);
uint32_t memory_alloc_frame(void); /* returns 0 when mapped frames are exhausted */
void memory_free_frame(uint32_t physical_address);
uint32_t memory_free_frames(void); /* free frames currently eligible for allocation */
uint32_t memory_total_frames(void); /* bitmap capacity: 4 GiB / 4 KiB */

#endif
