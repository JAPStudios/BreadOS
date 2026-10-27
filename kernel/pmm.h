#ifndef BREADOS_PMM_H
#define BREADOS_PMM_H

#include <stddef.h>
#include <stdint.h>

#define PMM_PAGE_SIZE 4096u
#define PMM_MAX_FRAMES 1048576u

int pmm_init(uint32_t multiboot_magic, uintptr_t multiboot_info_address);
int pmm_init_from_memory_map(const void *map, size_t length,
                             uintptr_t kernel_start,
                             uintptr_t kernel_end);
int pmm_reserve_range(uintptr_t start, size_t length);
uintptr_t pmm_alloc_page(void);
int pmm_free_page(uintptr_t physical_address);
uint32_t pmm_free_pages(void);

#endif
