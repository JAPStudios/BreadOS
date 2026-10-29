#include "gdt.h"
#include <stdint.h>
#include <stddef.h>

#define GDT_ENTRIES 6u
#define KERNEL_CODE_SELECTOR 0x08u
#define KERNEL_DATA_SELECTOR 0x10u
#define USER_CODE_SELECTOR   0x1Bu
#define USER_DATA_SELECTOR   0x23u
#define TSS_SELECTOR         0x28u

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss32 {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[GDT_ENTRIES];
static struct tss32 tss;
extern void gdt_load(const struct gdt_pointer *pointer);

static void set_entry(unsigned index, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t flags) {
    gdt[index].limit_low = (uint16_t)(limit & 0xFFFFu);
    gdt[index].base_low = (uint16_t)(base & 0xFFFFu);
    gdt[index].base_middle = (uint8_t)((base >> 16) & 0xFFu);
    gdt[index].access = access;
    gdt[index].granularity = (uint8_t)(((limit >> 16) & 0x0Fu) | (flags & 0xF0u));
    gdt[index].base_high = (uint8_t)((base >> 24) & 0xFFu);
}

void gdt_init(void) {
    for (unsigned i = 0; i < GDT_ENTRIES; ++i) {
        set_entry(i, 0, 0, 0, 0);
    }

    /* Flat 4 GiB segments. DPL=0 for kernel; DPL=3 for user segments. */
    set_entry(1, 0, 0xFFFFFu, 0x9Au, 0xC0u);
    set_entry(2, 0, 0xFFFFFu, 0x92u, 0xC0u);
    set_entry(3, 0, 0xFFFFFu, 0xFAu, 0xC0u);
    set_entry(4, 0, 0xFFFFFu, 0xF2u, 0xC0u);

    for (size_t i = 0; i < sizeof(tss); ++i) {
        ((uint8_t *)&tss)[i] = 0;
    }
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iomap_base = (uint16_t)sizeof(tss);
    set_entry(5, (uint32_t)(uintptr_t)&tss,
              (uint32_t)sizeof(tss) - 1u, 0x89u, 0x00u);

    const struct gdt_pointer pointer = {
        (uint16_t)(sizeof(gdt) - 1u),
        (uint32_t)(uintptr_t)gdt
    };
    gdt_load(&pointer);
}
