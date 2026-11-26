#include "gdt.h"
#include <stdint.h>
#include <stddef.h>

#define GDT_ENTRIES 10u
#define KERNEL_DATA_SELECTOR 0x10u
#define USER_TASK_FIRST 3u
#define USER_TASK_END 8u
#define USER_CODE_MIN 0x40000000u
#define USER_CODE_MAX 0x403FFFFFu
#define LDT_ENTRIES 8192u

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
static struct gdt_entry user_ldt[USER_TASK_END-USER_TASK_FIRST][LDT_ENTRIES];
static struct tss32 tss;
static uint16_t next_user_code_index=1u;
static uint16_t current_user_code_index[USER_TASK_END-USER_TASK_FIRST];
extern void gdt_load(const struct gdt_pointer *pointer);

static void set_entry(struct gdt_entry *table,unsigned index,
                      uint32_t base,uint32_t limit,
                      uint8_t access,uint8_t flags) {
    table[index].limit_low=(uint16_t)(limit&0xFFFFu);
    table[index].base_low=(uint16_t)(base&0xFFFFu);
    table[index].base_middle=(uint8_t)((base>>16)&0xFFu);
    table[index].access=access;
    table[index].granularity=(uint8_t)(((limit>>16)&0x0Fu)|(flags&0xF0u));
    table[index].base_high=(uint8_t)((base>>24)&0xFFu);
}

static unsigned user_ldt_slot(uint32_t task_id) {
    if (task_id<USER_TASK_FIRST || task_id>=USER_TASK_END) return USER_TASK_END;
    return task_id-USER_TASK_FIRST;
}

/* GDT index 4 remains the shared flat user-data descriptor. */
static unsigned user_ldt_gdt_index(uint32_t task_id) {
    unsigned slot=user_ldt_slot(task_id);
    if (slot>=USER_TASK_END-USER_TASK_FIRST) return 0;
    return slot==0u ? 3u : slot+4u;
}

void gdt_set_kernel_stack(uint32_t stack_pointer) {
    tss.esp0=stack_pointer;
}

/*
 * Allocate a fresh LDT selector for every process creation. Selector indices
 * are not reused during a boot, avoiding stale hidden-descriptor-cache aliases.
 * Only the active selector is present in that task's LDT.
 */
uint16_t gdt_prepare_user_code(uint32_t task_id,uint32_t inclusive_limit) {
    unsigned slot=user_ldt_slot(task_id);
    if (slot>=USER_TASK_END-USER_TASK_FIRST ||
        inclusive_limit<USER_CODE_MIN || inclusive_limit>USER_CODE_MAX ||
        next_user_code_index>=LDT_ENTRIES)
        return 0;

    if (current_user_code_index[slot])
        set_entry(user_ldt[slot],current_user_code_index[slot],0,0,0,0);

    uint16_t index=next_user_code_index++;
    uint32_t encoded=inclusive_limit>>12;
    set_entry(user_ldt[slot],index,0,encoded,0xFAu,0xC0u);
    current_user_code_index[slot]=index;
    return (uint16_t)((index<<3)|0x07u); /* LDT selector, RPL 3 */
}

void gdt_release_user_code(uint32_t task_id) {
    unsigned slot=user_ldt_slot(task_id);
    if (slot>=USER_TASK_END-USER_TASK_FIRST) return;
    if (current_user_code_index[slot]) {
        set_entry(user_ldt[slot],current_user_code_index[slot],0,0,0,0);
        current_user_code_index[slot]=0;
    }
}

/* Load this task's private LDT, or clear LDTR for a kernel task. */
void gdt_load_task_ldt(uint32_t task_id) {
    unsigned index=user_ldt_gdt_index(task_id);
    uint16_t selector=index ? (uint16_t)(index<<3) : 0u;
    __asm__ volatile ("lldt %0" : : "rm"(selector) : "memory");
}

void gdt_init(void) {
    for (unsigned i=0;i<GDT_ENTRIES;i++)
        set_entry(gdt,i,0,0,0,0);

    /* Flat kernel code/data and shared flat user data. */
    set_entry(gdt,1,0,0xFFFFFu,0x9Au,0xC0u);
    set_entry(gdt,2,0,0xFFFFFu,0x92u,0xC0u);
    set_entry(gdt,4,0,0xFFFFFu,0xF2u,0xC0u);

    for (uint32_t task_id=USER_TASK_FIRST;task_id<USER_TASK_END;task_id++) {
        unsigned slot=user_ldt_slot(task_id);
        unsigned gdt_index=user_ldt_gdt_index(task_id);
        for (unsigned entry=0;entry<LDT_ENTRIES;entry++)
            set_entry(user_ldt[slot],entry,0,0,0,0);
        /* LDT system descriptors are DPL 0 and cannot be loaded by ring-3 code. */
        set_entry(gdt,gdt_index,(uint32_t)(uintptr_t)user_ldt[slot],
                  (uint32_t)sizeof(user_ldt[slot])-1u,0x82u,0x00u);
    }

    for (size_t i=0;i<sizeof(tss);i++)
        ((uint8_t *)&tss)[i]=0;
    tss.ss0=KERNEL_DATA_SELECTOR;
    tss.iomap_base=(uint16_t)sizeof(tss);
    set_entry(gdt,9,(uint32_t)(uintptr_t)&tss,
              (uint32_t)sizeof(tss)-1u,0x89u,0x00u);

    const struct gdt_pointer pointer={
        (uint16_t)(sizeof(gdt)-1u),
        (uint32_t)(uintptr_t)gdt
    };
    gdt_load(&pointer);
}
