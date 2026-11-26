#include "gdt.h"
#include <stdint.h>
#include <stddef.h>
#define GDT_ENTRIES 6u
#define KERNEL_DATA_SELECTOR 0x10u
struct gdt_entry { uint16_t limit_low,base_low; uint8_t base_middle,access,granularity,base_high; } __attribute__((packed));
struct gdt_pointer { uint16_t limit; uint32_t base; } __attribute__((packed));
struct tss32 {
 uint32_t prev_tss,esp0,ss0,esp1,ss1,esp2,ss2,cr3,eip,eflags;
 uint32_t eax,ecx,edx,ebx,esp,ebp,esi,edi;
 uint32_t es,cs,ss,ds,fs,gs,ldt; uint16_t trap,iomap_base;
} __attribute__((packed));
static struct gdt_entry gdt[GDT_ENTRIES];
static struct tss32 tss;
extern void gdt_load(const struct gdt_pointer *pointer);
static void set_entry(unsigned i,uint32_t base,uint32_t limit,uint8_t access,uint8_t flags) {
 gdt[i].limit_low=(uint16_t)(limit&0xFFFFu); gdt[i].base_low=(uint16_t)(base&0xFFFFu);
 gdt[i].base_middle=(uint8_t)(base>>16); gdt[i].access=access;
 gdt[i].granularity=(uint8_t)(((limit>>16)&0x0Fu)|(flags&0xF0u)); gdt[i].base_high=(uint8_t)(base>>24);
}
void gdt_set_kernel_stack(uint32_t stack_pointer) { tss.esp0=stack_pointer; }
void gdt_init(void) {
 for(unsigned i=0;i<GDT_ENTRIES;i++) set_entry(i,0,0,0,0);
 set_entry(1,0,0xFFFFFu,0x9Au,0xC0u);
 set_entry(2,0,0xFFFFFu,0x92u,0xC0u);
 set_entry(3,0,0xFFFFFu,0xFAu,0xC0u);
 set_entry(4,0,0xFFFFFu,0xF2u,0xC0u);
 for(size_t i=0;i<sizeof(tss);i++) ((uint8_t *)&tss)[i]=0;
 tss.ss0=KERNEL_DATA_SELECTOR; tss.iomap_base=(uint16_t)sizeof(tss);
 set_entry(5,(uint32_t)(uintptr_t)&tss,(uint32_t)sizeof(tss)-1u,0x89u,0);
 const struct gdt_pointer p={(uint16_t)(sizeof(gdt)-1u),(uint32_t)(uintptr_t)gdt};
 gdt_load(&p);
}
