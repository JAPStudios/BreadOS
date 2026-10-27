#include "pmm.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_MEMORY_MAP (1u << 6)
#define LOW_MEMORY_BYTES 0x00100000u
#define MAX_PHYSICAL_BYTES ((uint64_t)PMM_MAX_FRAMES * PMM_PAGE_SIZE)
#define BITMAP_BYTES (PMM_MAX_FRAMES / 8u)
#define FIRST_ALLOCATABLE_FRAME (LOW_MEMORY_BYTES / PMM_PAGE_SIZE)

typedef struct __attribute__((packed)) {
    uint32_t size;
    uint64_t address;
    uint64_t length;
    uint32_t type;
} MultibootMemoryMapEntry;

typedef struct __attribute__((packed)) {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t command_line;
    uint32_t modules_count;
    uint32_t modules_address;
    uint32_t symbols[4];
    uint32_t memory_map_length;
    uint32_t memory_map_address;
} MultibootInfo;

static uint8_t used_bitmap[BITMAP_BYTES];
static uint8_t reserved_bitmap[BITMAP_BYTES];
static uint32_t free_count;
static uint32_t search_hint;
static int allocator_ready;

extern unsigned char __kernel_start[];
extern unsigned char __kernel_end[];

static int bit_get(const uint8_t *bitmap, uint32_t frame)
{
    return (bitmap[frame >> 3] >> (frame & 7u)) & 1u;
}

static void bit_set(uint8_t *bitmap, uint32_t frame)
{
    bitmap[frame >> 3] |= (uint8_t)(1u << (frame & 7u));
}

static void bit_clear(uint8_t *bitmap, uint32_t frame)
{
    bitmap[frame >> 3] &= (uint8_t)~(1u << (frame & 7u));
}

static void disable_allocator(void)
{
    for (size_t i = 0; i < BITMAP_BYTES; ++i) {
        used_bitmap[i] = 0xFFu;
        reserved_bitmap[i] = 0xFFu;
    }
    free_count = 0;
    search_hint = PMM_MAX_FRAMES;
    allocator_ready = 0;
}

/* Multiboot v1 entries include a size field followed by at least 20 bytes. */
static int map_valid(const uint8_t *bytes, size_t length)
{
    if (!bytes || length < sizeof(uint32_t) + 20u)
        return 0;

    size_t offset = 0;
    unsigned entries = 0;
    while (offset < length) {
        if (length - offset < sizeof(uint32_t))
            return 0;

        uint32_t size = *(const uint32_t *)(const void *)(bytes + offset);
        if (size < 20u || (size_t)size > length - offset - sizeof(uint32_t))
            return 0;

        const MultibootMemoryMapEntry *entry =
            (const MultibootMemoryMapEntry *)(const void *)(bytes + offset);
        if (entry->address + entry->length < entry->address)
            return 0;

        offset += sizeof(uint32_t) + (size_t)size;
        ++entries;
    }
    return offset == length && entries != 0;
}

static void reserve_range_internal(uint64_t start, uint64_t length)
{
    if (!length || start >= MAX_PHYSICAL_BYTES)
        return;

    uint64_t end = start + length;
    if (end < start || end > MAX_PHYSICAL_BYTES)
        end = MAX_PHYSICAL_BYTES;

    uint64_t first = start / PMM_PAGE_SIZE;
    uint64_t limit = (end + PMM_PAGE_SIZE - 1u) / PMM_PAGE_SIZE;
    if (limit > PMM_MAX_FRAMES)
        limit = PMM_MAX_FRAMES;

    for (uint32_t frame = (uint32_t)first;
         frame < (uint32_t)limit; ++frame) {
        if (!bit_get(used_bitmap, frame)) {
            bit_set(used_bitmap, frame);
            if (free_count)
                --free_count;
        }
        bit_set(reserved_bitmap, frame);
    }
}

static void mark_available_range(uint64_t start, uint64_t length)
{
    if (!length || start >= MAX_PHYSICAL_BYTES)
        return;

    uint64_t end = start + length;
    if (end < start)
        return;
    if (end > MAX_PHYSICAL_BYTES)
        end = MAX_PHYSICAL_BYTES;

    /* Make only complete pages available. */
    uint64_t first = (start + PMM_PAGE_SIZE - 1u) / PMM_PAGE_SIZE;
    uint64_t limit = end / PMM_PAGE_SIZE;
    if (first >= limit)
        return;

    for (uint32_t frame = (uint32_t)first;
         frame < (uint32_t)limit; ++frame) {
        bit_clear(reserved_bitmap, frame);
        if (bit_get(used_bitmap, frame)) {
            bit_clear(used_bitmap, frame);
            ++free_count;
        }
    }
}

int pmm_init_from_memory_map(const void *map, size_t length,
                             uintptr_t kernel_start,
                             uintptr_t kernel_end)
{
    disable_allocator();
    if (kernel_end < kernel_start ||
        !map_valid((const uint8_t *)map, length))
        return -1;

    const uint8_t *bytes = (const uint8_t *)map;
    size_t offset = 0;

    /* First mark usable (type 1) regions. */
    while (offset < length) {
        const MultibootMemoryMapEntry *entry =
            (const MultibootMemoryMapEntry *)(const void *)(bytes + offset);
        if (entry->type == 1u)
            mark_available_range(entry->address, entry->length);
        offset += sizeof(uint32_t) + (size_t)entry->size;
    }

    /* Non-usable entries override every usable region. */
    offset = 0;
    while (offset < length) {
        const MultibootMemoryMapEntry *entry =
            (const MultibootMemoryMapEntry *)(const void *)(bytes + offset);
        if (entry->type != 1u)
            reserve_range_internal(entry->address, entry->length);
        offset += sizeof(uint32_t) + (size_t)entry->size;
    }

    reserve_range_internal(0, LOW_MEMORY_BYTES);
    reserve_range_internal(kernel_start, (uint64_t)kernel_end - kernel_start);

    allocator_ready = 1;
    search_hint = FIRST_ALLOCATABLE_FRAME;
    if (!free_count) {
        disable_allocator();
        return -1;
    }
    return 0;
}

int pmm_init(uint32_t multiboot_magic, uintptr_t multiboot_info_address)
{
    disable_allocator();
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC ||
        !multiboot_info_address)
        return -1;

    const MultibootInfo *info =
        (const MultibootInfo *)(uintptr_t)multiboot_info_address;
    if (!(info->flags & MULTIBOOT_INFO_MEMORY_MAP) ||
        !info->memory_map_address || !info->memory_map_length)
        return -1;

    if (pmm_init_from_memory_map(
            (const void *)(uintptr_t)info->memory_map_address,
            info->memory_map_length,
            (uintptr_t)__kernel_start,
            (uintptr_t)__kernel_end) < 0)
        return -1;

    if (pmm_reserve_range(multiboot_info_address, sizeof(*info)) < 0 ||
        pmm_reserve_range(info->memory_map_address,
                          info->memory_map_length) < 0) {
        disable_allocator();
        return -1;
    }
    return 0;
}

int pmm_reserve_range(uintptr_t start, size_t length)
{
    if (!allocator_ready)
        return -1;
    reserve_range_internal((uint64_t)start, (uint64_t)length);
    return 0;
}

uintptr_t pmm_alloc_page(void)
{
    if (!allocator_ready || !free_count)
        return 0;

    uint32_t start = search_hint;
    if (start < FIRST_ALLOCATABLE_FRAME || start >= PMM_MAX_FRAMES)
        start = FIRST_ALLOCATABLE_FRAME;

    for (unsigned pass = 0; pass < 2; ++pass) {
        uint32_t begin = pass == 0 ? start : FIRST_ALLOCATABLE_FRAME;
        uint32_t limit = pass == 0 ? PMM_MAX_FRAMES : start;

        for (uint32_t frame = begin; frame < limit; ++frame) {
            if (!bit_get(used_bitmap, frame) &&
                !bit_get(reserved_bitmap, frame)) {
                bit_set(used_bitmap, frame);
                --free_count;
                search_hint = frame + 1u;
                if (search_hint >= PMM_MAX_FRAMES)
                    search_hint = FIRST_ALLOCATABLE_FRAME;
                return (uintptr_t)((uint64_t)frame * PMM_PAGE_SIZE);
            }
        }
    }
    return 0;
}

int pmm_free_page(uintptr_t physical_address)
{
    if (!allocator_ready ||
        (physical_address & (PMM_PAGE_SIZE - 1u)) != 0)
        return -1;

    uint32_t frame = (uint32_t)(physical_address / PMM_PAGE_SIZE);
    if (frame < FIRST_ALLOCATABLE_FRAME || frame >= PMM_MAX_FRAMES)
        return -1;
    if (bit_get(reserved_bitmap, frame) || !bit_get(used_bitmap, frame))
        return -1;

    bit_clear(used_bitmap, frame);
    ++free_count;
    if (frame < search_hint)
        search_hint = frame;
    return 0;
}

uint32_t pmm_free_pages(void)
{
    return allocator_ready ? free_count : 0;
}
