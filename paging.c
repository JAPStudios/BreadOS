#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "memory.h"

#define PAGE_SIZE 4096u
#define PAGE_ENTRIES 1024u
#define IDENTITY_MAP_BYTES (256u * 1024u * 1024u)
#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u
#define PAGE_USER 0x004u
#define PAGE_WRITE_THROUGH 0x008u
#define PAGE_CACHE_DISABLE 0x010u
#define FB_VIRTUAL_BASE 0xE0000000u
#define FB_PDE_INDEX (FB_VIRTUAL_BASE >> 22)
#define USER_REGION_START 0x40000000u
#define USER_REGION_END 0x40400000u
#define USER_PDE_INDEX (USER_REGION_START >> 22)

static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_tables[IDENTITY_MAP_BYTES / PAGE_SIZE / PAGE_ENTRIES]
                           [PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t framebuffer_table[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static int paging_enabled;
static uint32_t active_directory_physical;

static void invalidate_page(uint32_t address) {
    __asm__ volatile("invlpg (%0)" : : "r"((void *)(uintptr_t)address) : "memory");
}

void paging_init(void) {
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) page_directory[i] = 0;
    const uint32_t table_count = (IDENTITY_MAP_BYTES / PAGE_SIZE) / PAGE_ENTRIES;
    for (uint32_t table = 0; table < table_count; ++table) {
        for (uint32_t page = 0; page < PAGE_ENTRIES; ++page) {
            uint32_t physical = (table * PAGE_ENTRIES + page) * PAGE_SIZE;
            page_tables[table][page] = physical | PAGE_PRESENT | PAGE_WRITABLE;
        }
        page_directory[table] = (uint32_t)(uintptr_t)&page_tables[table][0] |
                                PAGE_PRESENT | PAGE_WRITABLE;
    }
    active_directory_physical = (uint32_t)(uintptr_t)page_directory;
    __asm__ volatile("mov %0, %%cr3" : : "r"(active_directory_physical) : "memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80010000u; /* Enable paging and supervisor write protection. */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
    paging_enabled = 1;
}

void *paging_map_framebuffer(uint64_t physical_address, uint32_t length) {
    if (!paging_enabled || !length || physical_address > 0xFFFFFFFFULL) return NULL;
    uint64_t offset = physical_address & (PAGE_SIZE - 1u);
    uint64_t base = physical_address - offset;
    uint64_t span = offset + (uint64_t)length;
    uint64_t pages = (span + PAGE_SIZE - 1u) / PAGE_SIZE;
    if (!pages || pages > PAGE_ENTRIES || base + pages * PAGE_SIZE > 0x100000000ULL)
        return NULL;
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) framebuffer_table[i] = 0;
    for (uint32_t i = 0; i < (uint32_t)pages; ++i) {
        uint32_t physical = (uint32_t)(base + (uint64_t)i * PAGE_SIZE);
        framebuffer_table[i] = physical | PAGE_PRESENT | PAGE_WRITABLE |
                                PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE;
    }
    page_directory[FB_PDE_INDEX] = (uint32_t)(uintptr_t)framebuffer_table |
                                   PAGE_PRESENT | PAGE_WRITABLE;
    __asm__ volatile("mov %0, %%cr3" : : "r"(active_directory_physical) : "memory");
    return (void *)(uintptr_t)(FB_VIRTUAL_BASE + (uint32_t)offset);
}

int paging_address_space_create(struct paging_address_space *space) {
    if (!paging_enabled || !space || space->directory_physical ||
        space->user_table_physical) return -1;
    space->directory_physical = 0;
    space->user_table_physical = 0;

    uint32_t directory_frame = memory_alloc_frame();
    if (!directory_frame) return -1;
    uint32_t table_frame = memory_alloc_frame();
    if (!table_frame) {
        memory_free_frame(directory_frame);
        return -1;
    }

    uint32_t *directory = (uint32_t *)(uintptr_t)directory_frame;
    uint32_t *user_table = (uint32_t *)(uintptr_t)table_frame;
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) {
        directory[i] = page_directory[i];
        user_table[i] = 0;
    }
    directory[USER_PDE_INDEX] = table_frame | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    space->directory_physical = directory_frame;
    space->user_table_physical = table_frame;
    return 0;
}

void paging_address_space_destroy(struct paging_address_space *space) {
    if (!space || !space->directory_physical || !space->user_table_physical) return;
    if (active_directory_physical == space->directory_physical)
        paging_switch_kernel_address_space();
    memory_free_frame(space->user_table_physical);
    memory_free_frame(space->directory_physical);
    space->directory_physical = 0;
    space->user_table_physical = 0;
}

int paging_address_space_map_user_page(struct paging_address_space *space,
                                       uint32_t virtual_address,
                                       uint32_t physical_address,
                                       int writable) {
    if (!paging_enabled || !space || !space->directory_physical ||
        !space->user_table_physical ||
        (virtual_address & (PAGE_SIZE - 1u)) ||
        (physical_address & (PAGE_SIZE - 1u)) || physical_address == 0 ||
        virtual_address < USER_REGION_START || virtual_address >= USER_REGION_END ||
        physical_address >= IDENTITY_MAP_BYTES ||
        (writable != 0 && writable != 1))
        return -1;

    uint32_t *table = (uint32_t *)(uintptr_t)space->user_table_physical;
    uint32_t index = (virtual_address >> 12) & 0x3FFu;
    if (table[index] & PAGE_PRESENT) return -1;

    uint32_t flags = PAGE_PRESENT | PAGE_USER;
    if (writable) flags |= PAGE_WRITABLE;
    table[index] = physical_address | flags;
    if (active_directory_physical == space->directory_physical)
        invalidate_page(virtual_address);
    return 0;
}

void paging_address_space_unmap_user_page(struct paging_address_space *space,
                                          uint32_t virtual_address) {
    if (!space || !space->directory_physical || !space->user_table_physical ||
        (virtual_address & (PAGE_SIZE - 1u)) ||
        virtual_address < USER_REGION_START || virtual_address >= USER_REGION_END)
        return;

    uint32_t *table = (uint32_t *)(uintptr_t)space->user_table_physical;
    uint32_t index = (virtual_address >> 12) & 0x3FFu;
    table[index] = 0;
    if (active_directory_physical == space->directory_physical)
        invalidate_page(virtual_address);
}

int paging_switch_address_space(const struct paging_address_space *space) {
    if (!paging_enabled || !space || !space->directory_physical ||
        !space->user_table_physical ||
        (space->directory_physical & (PAGE_SIZE - 1u)) ||
        (space->user_table_physical & (PAGE_SIZE - 1u)))
        return -1;
    active_directory_physical = space->directory_physical;
    __asm__ volatile("mov %0, %%cr3" : : "r"(active_directory_physical) : "memory");
    return 0;
}

void paging_switch_kernel_address_space(void) {
    if (!paging_enabled) return;
    active_directory_physical = (uint32_t)(uintptr_t)page_directory;
    __asm__ volatile("mov %0, %%cr3" : : "r"(active_directory_physical) : "memory");
}

int paging_user_range_valid(const struct paging_address_space *space,
                            uint32_t address,uint32_t length,int writable) {
    if (!space || !space->user_table_physical || !length ||
        (writable!=0 && writable!=1) ||
        address<USER_REGION_START ||
        (uint64_t)address+(uint64_t)length>USER_REGION_END)
        return 0;
    const uint32_t *table=(const uint32_t *)(uintptr_t)space->user_table_physical;
    uint32_t first=address&~(PAGE_SIZE-1u);
    uint32_t last=(address+length-1u)&~(PAGE_SIZE-1u);
    for (uint32_t page=first;;page+=PAGE_SIZE) {
        uint32_t entry=table[(page>>12)&0x3FFu];
        if (!(entry&PAGE_PRESENT) || !(entry&PAGE_USER) ||
            (writable && !(entry&PAGE_WRITABLE)))
            return 0;
        if (page==last) break;
        if (page>0xFFFFFFFFu-PAGE_SIZE) return 0;
    }
    return 1;
}
