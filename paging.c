#include <stdint.h>
#include "paging.h"

#define PAGE_SIZE 4096u
#define PAGE_ENTRIES 1024u
#define IDENTITY_MAP_BYTES (16u * 1024u * 1024u)
#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u

/*
 * These tables live in the kernel image, below 16 MiB, and are identity
 * mapped before paging is enabled. This is an initial bootstrap map, not
 * per-process address-space isolation.
 */
static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_tables[IDENTITY_MAP_BYTES / PAGE_SIZE / PAGE_ENTRIES]
                           [PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

void paging_init(void) {
    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i) {
        page_directory[i] = 0;
    }

    const uint32_t table_count =
        (IDENTITY_MAP_BYTES / PAGE_SIZE) / PAGE_ENTRIES;
    for (uint32_t table = 0; table < table_count; ++table) {
        for (uint32_t page = 0; page < PAGE_ENTRIES; ++page) {
            uint32_t physical = (table * PAGE_ENTRIES + page) * PAGE_SIZE;
            page_tables[table][page] = physical | PAGE_PRESENT | PAGE_WRITABLE;
        }
        page_directory[table] =
            (uint32_t)(uintptr_t)&page_tables[table][0] |
            PAGE_PRESENT | PAGE_WRITABLE;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)page_directory) : "memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u; /* CR0.PG: enable paging */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}
