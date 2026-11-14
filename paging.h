#ifndef BREADOS_PAGING_H
#define BREADOS_PAGING_H
#include <stdint.h>

/* Enables supervisor-only, writable identity mapping of the first 16 MiB. */
void paging_init(void);

/* Maps a physical framebuffer into a dedicated 4 MiB virtual window. */
void *paging_map_framebuffer(uint64_t physical_address, uint32_t length);

/* A process owns a private page directory and private user page table. */
struct paging_address_space {
    uint32_t directory_physical;
    uint32_t user_table_physical;
};

int paging_address_space_create(struct paging_address_space *space);
void paging_address_space_destroy(struct paging_address_space *space);
int paging_address_space_map_user_page(struct paging_address_space *space,
                                       uint32_t virtual_address,
                                       uint32_t physical_address,
                                       int writable);
void paging_address_space_unmap_user_page(struct paging_address_space *space,
                                          uint32_t virtual_address);
int paging_switch_address_space(const struct paging_address_space *space);
void paging_switch_kernel_address_space(void);

#endif
