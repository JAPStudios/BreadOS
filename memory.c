#include "memory.h"
#include <stddef.h>

#define PAGE_SIZE 4096u
#define FRAME_COUNT 1048576u /* 4 GiB / 4 KiB */
#define BITMAP_WORDS (FRAME_COUNT / 32u)
#define MB_FLAG_MEMORY  (1u << 0)
#define MB_FLAG_MMAP    (1u << 6)

struct multiboot_info {
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline;
    uint32_t mods_count, mods_addr, syms[4], mmap_length, mmap_addr;
} __attribute__((packed));
struct mmap_entry {
    uint32_t size;
    uint32_t base_low, base_high, length_low, length_high, type;
} __attribute__((packed));

static uint32_t bitmap[BITMAP_WORDS];
static uint32_t free_count;
static uint32_t scan_hint;
extern uint8_t kernel_end[];

static void mark_used(uint32_t frame) {
    uint32_t mask = 1u << (frame & 31u);
    uint32_t *word = &bitmap[frame >> 5];
    if (!(*word & mask)) { *word |= mask; if (free_count) free_count--; }
}
static void mark_free(uint32_t frame) {
    uint32_t mask = 1u << (frame & 31u);
    uint32_t *word = &bitmap[frame >> 5];
    if (*word & mask) { *word &= ~mask; free_count++; }
}
static void reserve_range(uint64_t start, uint64_t length) {
    uint64_t end = start + length;
    if (end < start || end > 0x100000000ULL) end = 0x100000000ULL;
    uint32_t first = (uint32_t)(start / PAGE_SIZE);
    uint32_t last = (uint32_t)((end + PAGE_SIZE - 1u) / PAGE_SIZE);
    if (last > FRAME_COUNT) last = FRAME_COUNT;
    for (uint32_t f = first; f < last; ++f) mark_used(f);
}
static void usable_range(uint64_t start, uint64_t length) {
    uint64_t end = start + length;
    if (end < start || start >= 0x100000000ULL) return;
    if (end > 0x100000000ULL) end = 0x100000000ULL;
    uint64_t first = (start + PAGE_SIZE - 1u) / PAGE_SIZE;
    uint64_t last = end / PAGE_SIZE;
    if (last > FRAME_COUNT) last = FRAME_COUNT;
    for (uint64_t f = first; f < last; ++f) mark_free((uint32_t)f);
}
void memory_init(uint32_t info_address) {
    for (uint32_t i=0; i<BITMAP_WORDS; ++i) bitmap[i]=0xFFFFFFFFu;
    free_count=0; scan_hint=256;
    const struct multiboot_info *info=(const struct multiboot_info *)(uintptr_t)info_address;
    if (info && (info->flags & MB_FLAG_MMAP) && info->mmap_length) {
        uint32_t offset=0;
        while (offset + sizeof(uint32_t) <= info->mmap_length) {
            const struct mmap_entry *entry=(const struct mmap_entry *)(uintptr_t)(info->mmap_addr+offset);
            uint32_t step=entry->size+sizeof(entry->size);
            if (step < sizeof(*entry) || step > info->mmap_length-offset) break;
            uint64_t base=((uint64_t)entry->base_high<<32)|entry->base_low;
            uint64_t len=((uint64_t)entry->length_high<<32)|entry->length_low;
            if (entry->type==1 && len) usable_range(base,len);
            offset += step;
        }
        reserve_range(info_address,sizeof(*info));
        reserve_range(info->mmap_addr,info->mmap_length);
    } else if (info && (info->flags & MB_FLAG_MEMORY)) {
        usable_range(0x100000ULL,(uint64_t)info->mem_upper*1024ULL);
    }
    reserve_range(0,0x100000ULL);
    reserve_range(0,(uint64_t)(uintptr_t)kernel_end);
}
uint32_t memory_alloc_frame(void) {
    if (!free_count) return 0;
    for (uint32_t scanned=0; scanned<FRAME_COUNT; ++scanned) {
        uint32_t f=(scan_hint+scanned)%FRAME_COUNT;
        if (!(bitmap[f>>5] & (1u<<(f&31u)))) {
            mark_used(f);
            scan_hint=(f+1u)%FRAME_COUNT;
            return f*PAGE_SIZE;
        }
    }
    return 0;
}
void memory_free_frame(uint32_t address) {
    if ((address & (PAGE_SIZE-1u)) || address==0 || address>=0xFFFFF000u) return;
    uint32_t f=address/PAGE_SIZE;
    if (bitmap[f>>5] & (1u<<(f&31u))) mark_free(f);
}
uint32_t memory_free_frames(void) { return free_count; }
uint32_t memory_total_frames(void) { return FRAME_COUNT; }
