#include "memory.h"
#include <stddef.h>

#define PAGE_SIZE 4096u
#define FRAME_COUNT 1048576u /* Bitmap covers the 32-bit physical address space. */
#define BITMAP_WORDS (FRAME_COUNT / 32u)
/* paging.c currently identity-maps only the first 16 MiB. Until the direct
 * map is extended, never advertise or allocate frames outside that range. */
#define DIRECT_MAP_LIMIT (16u * 1024u * 1024u)
#define DIRECT_MAP_FRAMES (DIRECT_MAP_LIMIT / PAGE_SIZE)
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
    if (!(*word & mask)) { *word |= mask; if (frame < DIRECT_MAP_FRAMES && free_count) free_count--; }
}
static void mark_free(uint32_t frame) {
    if (frame >= DIRECT_MAP_FRAMES) return;
    uint32_t mask = 1u << (frame & 31u);
    uint32_t *word = &bitmap[frame >> 5];
    if (*word & mask) { *word &= ~mask; free_count++; }
}
static void reserve_range(uint64_t start, uint64_t length) {
    uint64_t end = start + length;
    if (end < start || end > 0x100000000ULL) end = 0x100000000ULL;
    uint64_t first64 = start / PAGE_SIZE;
    uint64_t last64 = (end + PAGE_SIZE - 1u) / PAGE_SIZE;
    if (first64 >= FRAME_COUNT) return;
    if (last64 > FRAME_COUNT) last64 = FRAME_COUNT;
    for (uint32_t f = (uint32_t)first64; f < (uint32_t)last64; ++f) mark_used(f);
}
static void usable_range(uint64_t start, uint64_t length) {
    uint64_t end = start + length;
    if (end < start || start >= DIRECT_MAP_LIMIT) return;
    if (end > DIRECT_MAP_LIMIT) end = DIRECT_MAP_LIMIT;
    uint64_t first = (start + PAGE_SIZE - 1u) / PAGE_SIZE;
    uint64_t last = end / PAGE_SIZE;
    if (last > DIRECT_MAP_FRAMES) last = DIRECT_MAP_FRAMES;
    for (uint64_t f = first; f < last; ++f) mark_free((uint32_t)f);
}
void memory_init(uint32_t info_address) {
    for (uint32_t i = 0; i < BITMAP_WORDS; ++i) bitmap[i] = 0xFFFFFFFFu;
    free_count = 0;
    scan_hint = 256u;
    const struct multiboot_info *info =
        (const struct multiboot_info *)(uintptr_t)info_address;
    if (info && (info->flags & MB_FLAG_MMAP) && info->mmap_length) {
        uint32_t offset = 0;
        while (offset + sizeof(uint32_t) <= info->mmap_length) {
            const struct mmap_entry *entry =
                (const struct mmap_entry *)(uintptr_t)(info->mmap_addr + offset);
            uint32_t step = entry->size + sizeof(entry->size);
            if (step < sizeof(*entry) || step > info->mmap_length - offset) break;
            uint64_t base = ((uint64_t)entry->base_high << 32) | entry->base_low;
            uint64_t len = ((uint64_t)entry->length_high << 32) | entry->length_low;
            if (entry->type == 1 && len) usable_range(base, len);
            offset += step;
        }
        reserve_range(info_address, sizeof(*info));
        reserve_range(info->mmap_addr, info->mmap_length);
    } else if (info && (info->flags & MB_FLAG_MEMORY)) {
        usable_range(0x100000ULL, (uint64_t)info->mem_upper * 1024ULL);
    }
    reserve_range(0, 0x100000ULL);
    reserve_range(0, (uint64_t)(uintptr_t)kernel_end);
}
uint32_t memory_alloc_frame(void) {
    if (!free_count) return 0;
    for (uint32_t scanned = 0; scanned < DIRECT_MAP_FRAMES; ++scanned) {
        uint32_t frame = (scan_hint + scanned) % DIRECT_MAP_FRAMES;
        if (!(bitmap[frame >> 5] & (1u << (frame & 31u)))) {
            mark_used(frame);
            scan_hint = (frame + 1u) % DIRECT_MAP_FRAMES;
            return frame * PAGE_SIZE;
        }
    }
    return 0;
}
void memory_free_frame(uint32_t address) {
    if ((address & (PAGE_SIZE - 1u)) || address == 0 ||
        address >= DIRECT_MAP_LIMIT) return;
    mark_free(address / PAGE_SIZE);
}
uint32_t memory_free_frames(void) { return free_count; }
uint32_t memory_total_frames(void) { return FRAME_COUNT; }
