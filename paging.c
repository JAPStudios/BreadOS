#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "memory.h"
#define PAGE_SIZE 4096u
#define PT_ENTRIES 512u
#define IDENTITY_MAP_BYTES (256u*1024u*1024u)
#define IDENTITY_PT_COUNT (IDENTITY_MAP_BYTES/(PAGE_SIZE*PT_ENTRIES))
#define P 0x001ull
#define RW 0x002ull
#define US 0x004ull
#define PWT 0x008ull
#define PCD 0x010ull
#define NX (1ull<<63)
#define FB_BASE 0xE0000000u
#define FB_PDPT (FB_BASE>>30)
#define FB_PDE (((FB_BASE>>21)&0x1FFu))
#define FB_PT_COUNT 2u
#define USER_BASE 0x40000000u
#define USER_END 0x40400000u
#define USER_PDPT (USER_BASE>>30)

static uint64_t kernel_pdpt[4] __attribute__((aligned(4096)));
static uint64_t kernel_pd0[PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t kernel_identity_pts[IDENTITY_PT_COUNT][PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t kernel_pd3[PT_ENTRIES] __attribute__((aligned(4096)));
static uint64_t framebuffer_pts[FB_PT_COUNT][PT_ENTRIES] __attribute__((aligned(4096)));
static int paging_enabled,nx_enabled;
static uint32_t active_pdpt;

static void cpuid(uint32_t leaf,uint32_t subleaf,uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d) {
    __asm__ volatile("cpuid":"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d):"a"(leaf),"c"(subleaf));
}
static int cpuid_available(void) {
    uint32_t before,after;
    __asm__ volatile("pushfl; popl %0":"=r"(before));
    uint32_t changed=before^(1u<<21);
    __asm__ volatile("pushl %0; popfl"::"r"(changed):"cc");
    __asm__ volatile("pushfl; popl %0":"=r"(after));
    __asm__ volatile("pushl %0; popfl"::"r"(before):"cc");
    return ((before^after)&(1u<<21))!=0;
}
static int has_pae(void) {
    uint32_t a,b,c,d;
    if(!cpuid_available()) return 0;
    cpuid(1,0,&a,&b,&c,&d);
    return (d&(1u<<6))!=0;
}
static int has_nx(void) {
    uint32_t a,b,c,d;
    cpuid(0x80000000u,0,&a,&b,&c,&d);
    if(a<0x80000001u) return 0;
    cpuid(0x80000001u,0,&a,&b,&c,&d);
    return (d&(1u<<20))!=0;
}
static void zero_table(uint64_t *p,uint32_t n) { for(uint32_t i=0;i<n;i++) p[i]=0; }
static void invlpg(uint32_t address) {
    __asm__ volatile("invlpg (%0)"::"r"((void *)(uintptr_t)address):"memory");
}

void paging_init(void) {
    paging_enabled=0; nx_enabled=0;
    if(!has_pae()) return;
    zero_table(kernel_pdpt,4);
    zero_table(kernel_pd0,PT_ENTRIES);
    zero_table(kernel_pd3,PT_ENTRIES);
    for(uint32_t t=0;t<IDENTITY_PT_COUNT;t++) {
        for(uint32_t p=0;p<PT_ENTRIES;p++) {
            uint32_t physical=(t*PT_ENTRIES+p)*PAGE_SIZE;
            kernel_identity_pts[t][p]=(uint64_t)physical|P|RW;
        }
        kernel_pd0[t]=(uint64_t)(uintptr_t)&kernel_identity_pts[t][0]|P|RW;
    }
    for(uint32_t t=0;t<FB_PT_COUNT;t++) zero_table(framebuffer_pts[t],PT_ENTRIES);
    kernel_pdpt[0]=(uint64_t)(uintptr_t)kernel_pd0|P;
    kernel_pdpt[FB_PDPT]=(uint64_t)(uintptr_t)kernel_pd3|P;
    uint32_t cr4;
    __asm__ volatile("mov %%cr4,%0":"=r"(cr4));
    cr4|=1u<<5;
    __asm__ volatile("mov %0,%%cr4"::"r"(cr4):"memory");
    nx_enabled=has_nx();
    if(nx_enabled) {
        uint32_t lo,hi;
        __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xC0000080u));
        lo|=1u<<11;
        __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(0xC0000080u):"memory");
    }
    active_pdpt=(uint32_t)(uintptr_t)kernel_pdpt;
    __asm__ volatile("mov %0,%%cr3"::"r"(active_pdpt):"memory");
    uint32_t cr0;
    __asm__ volatile("mov %%cr0,%0":"=r"(cr0));
    cr0|=0x80010000u;
    __asm__ volatile("mov %0,%%cr0"::"r"(cr0):"memory");
    paging_enabled=1;
}
int paging_nx_supported(void) { return paging_enabled && nx_enabled; }

void *paging_map_framebuffer(uint64_t physical_address,uint32_t length) {
    if(!paging_enabled || !length || physical_address>0xFFFFFFFFULL) return NULL;
    uint64_t offset=physical_address&(PAGE_SIZE-1u);
    uint64_t base=physical_address-offset;
    uint64_t pages=(offset+(uint64_t)length+PAGE_SIZE-1u)/PAGE_SIZE;
    if(!pages || pages>FB_PT_COUNT*PT_ENTRIES || base+pages*PAGE_SIZE>0x100000000ULL) return NULL;
    for(uint32_t t=0;t<FB_PT_COUNT;t++) {
        zero_table(framebuffer_pts[t],PT_ENTRIES);
        kernel_pd3[FB_PDE+t]=0;
    }
    uint64_t flags=P|RW|PWT|PCD|(nx_enabled?NX:0);
    for(uint32_t i=0;i<(uint32_t)pages;i++)
        framebuffer_pts[i/PT_ENTRIES][i%PT_ENTRIES]=(base+(uint64_t)i*PAGE_SIZE)|flags;
    uint32_t count=((uint32_t)pages+PT_ENTRIES-1u)/PT_ENTRIES;
    for(uint32_t t=0;t<count;t++)
        kernel_pd3[FB_PDE+t]=(uint64_t)(uintptr_t)&framebuffer_pts[t][0]|P|RW;
    __asm__ volatile("mov %0,%%cr3"::"r"(active_pdpt):"memory");
    return (void *)(uintptr_t)(FB_BASE+(uint32_t)offset);
}

int paging_address_space_create(struct paging_address_space *s) {
    if(!paging_nx_supported() || !s || s->directory_physical || s->user_directory_physical ||
       s->user_table_physical[0] || s->user_table_physical[1]) return -1;
    uint32_t pdptf=memory_alloc_frame(); if(!pdptf) return -1;
    uint32_t pdf=memory_alloc_frame(); if(!pdf) {memory_free_frame(pdptf);return -1;}
    uint32_t pt0f=memory_alloc_frame(); if(!pt0f) {memory_free_frame(pdf);memory_free_frame(pdptf);return -1;}
    uint32_t pt1f=memory_alloc_frame(); if(!pt1f) {memory_free_frame(pt0f);memory_free_frame(pdf);memory_free_frame(pdptf);return -1;}
    uint64_t *pdpt=(uint64_t *)(uintptr_t)pdptf;
    uint64_t *pd=(uint64_t *)(uintptr_t)pdf;
    uint64_t *pt0=(uint64_t *)(uintptr_t)pt0f;
    uint64_t *pt1=(uint64_t *)(uintptr_t)pt1f;
    for(uint32_t i=0;i<4;i++) pdpt[i]=kernel_pdpt[i];
    pdpt[USER_PDPT]=(uint64_t)pdf|P;
    zero_table(pd,PT_ENTRIES); zero_table(pt0,PT_ENTRIES); zero_table(pt1,PT_ENTRIES);
    pd[0]=(uint64_t)pt0f|P|RW|US;
    pd[1]=(uint64_t)pt1f|P|RW|US;
    s->directory_physical=pdptf; s->user_directory_physical=pdf;
    s->user_table_physical[0]=pt0f; s->user_table_physical[1]=pt1f;
    return 0;
}
void paging_address_space_destroy(struct paging_address_space *s) {
    if(!s || !s->directory_physical || !s->user_directory_physical ||
       !s->user_table_physical[0] || !s->user_table_physical[1]) return;
    if(active_pdpt==s->directory_physical) paging_switch_kernel_address_space();
    memory_free_frame(s->user_table_physical[0]); memory_free_frame(s->user_table_physical[1]);
    memory_free_frame(s->user_directory_physical); memory_free_frame(s->directory_physical);
    s->directory_physical=0; s->user_directory_physical=0;
    s->user_table_physical[0]=0; s->user_table_physical[1]=0;
}
int paging_address_space_map_user_page(struct paging_address_space *s,uint32_t va,
                                       uint32_t pa,int writable,int executable) {
    if(!paging_nx_supported() || !s || !s->directory_physical ||
       !s->user_directory_physical || !s->user_table_physical[0] || !s->user_table_physical[1] ||
       (va&(PAGE_SIZE-1u)) || (pa&(PAGE_SIZE-1u)) || !pa || pa>=IDENTITY_MAP_BYTES ||
       va<USER_BASE || va>=USER_END || (writable!=0 && writable!=1) ||
       (executable!=0 && executable!=1) || (writable && executable)) return -1;
    uint32_t off=va-USER_BASE,ti=off>>21,pi=(va>>12)&0x1FFu;
    uint64_t *pt=(uint64_t *)(uintptr_t)s->user_table_physical[ti];
    if(pt[pi]&P) return -1;
    uint64_t flags=P|US|(writable?RW:0)|(!executable?NX:0);
    pt[pi]=(uint64_t)pa|flags;
    if(active_pdpt==s->directory_physical) invlpg(va);
    return 0;
}
void paging_address_space_unmap_user_page(struct paging_address_space *s,uint32_t va) {
    if(!s || !s->directory_physical || !s->user_table_physical[0] || !s->user_table_physical[1] ||
       (va&(PAGE_SIZE-1u)) || va<USER_BASE || va>=USER_END) return;
    uint32_t off=va-USER_BASE,ti=off>>21,pi=(va>>12)&0x1FFu;
    ((uint64_t *)(uintptr_t)s->user_table_physical[ti])[pi]=0;
    if(active_pdpt==s->directory_physical) invlpg(va);
}
int paging_switch_address_space(const struct paging_address_space *s) {
    if(!paging_enabled || !s || !s->directory_physical || !s->user_directory_physical ||
       !s->user_table_physical[0] || !s->user_table_physical[1] ||
       (s->directory_physical&(PAGE_SIZE-1u))) return -1;
    active_pdpt=s->directory_physical;
    __asm__ volatile("mov %0,%%cr3"::"r"(active_pdpt):"memory");
    return 0;
}
void paging_switch_kernel_address_space(void) {
    if(!paging_enabled) return;
    active_pdpt=(uint32_t)(uintptr_t)kernel_pdpt;
    __asm__ volatile("mov %0,%%cr3"::"r"(active_pdpt):"memory");
}
int paging_user_range_valid(const struct paging_address_space *s,uint32_t address,
                            uint32_t length,int writable) {
    if(!s || !s->user_table_physical[0] || !s->user_table_physical[1] || !length ||
       (writable!=0 && writable!=1) || address<USER_BASE ||
       (uint64_t)address+length>USER_END) return 0;
    uint64_t first=(uint64_t)address&~((uint64_t)PAGE_SIZE-1u);
    uint64_t last=((uint64_t)address+length-1u)&~((uint64_t)PAGE_SIZE-1u);
    for(uint64_t page=first;;page+=PAGE_SIZE) {
        uint32_t off=(uint32_t)(page-USER_BASE),ti=off>>21,pi=((uint32_t)page>>12)&0x1FFu;
        uint64_t entry=((uint64_t *)(uintptr_t)s->user_table_physical[ti])[pi];
        if(!(entry&P) || !(entry&US) || (writable && !(entry&RW))) return 0;
        if(page==last) break;
    }
    return 1;
}
