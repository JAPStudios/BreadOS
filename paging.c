#include <stdint.h>
#include <stddef.h>
#include "paging.h"

#define PAGE_SIZE 4096u
#define PAGE_ENTRIES 1024u
#define IDENTITY_MAP_BYTES (16u * 1024u * 1024u)
#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u
#define PAGE_WRITE_THROUGH 0x008u
#define PAGE_CACHE_DISABLE 0x010u
#define FB_VIRTUAL_BASE 0xE0000000u
#define FB_PDE_INDEX (FB_VIRTUAL_BASE >> 22)

static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_tables[IDENTITY_MAP_BYTES / PAGE_SIZE / PAGE_ENTRIES]
                           [PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t framebuffer_table[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static int paging_enabled;

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
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)page_directory) : "memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
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
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)page_directory) : "memory");
    return (void *)(uintptr_t)(FB_VIRTUAL_BASE + (uint32_t)offset);
}
