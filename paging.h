#ifndef BREADOS_PAGING_H
#define BREADOS_PAGING_H
#include <stdint.h>
void paging_init(void);
int paging_nx_supported(void);
void *paging_map_framebuffer(uint64_t physical_address,uint32_t length);
struct paging_address_space {
    uint32_t directory_physical;
    uint32_t user_directory_physical;
    uint32_t user_table_physical[2];
};
int paging_address_space_create(struct paging_address_space *space);
void paging_address_space_destroy(struct paging_address_space *space);
int paging_address_space_map_user_page(struct paging_address_space *space,
                                       uint32_t virtual_address,uint32_t physical_address,
                                       int writable,int executable);
void paging_address_space_unmap_user_page(struct paging_address_space *space,uint32_t virtual_address);
int paging_switch_address_space(const struct paging_address_space *space);
void paging_switch_kernel_address_space(void);
int paging_user_range_valid(const struct paging_address_space *space,
                            uint32_t address,uint32_t length,int writable);
#endif
