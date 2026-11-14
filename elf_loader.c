#include "elf_loader.h"

#define ELF_HEADER_SIZE 52u
#define ELF_PROGRAM_HEADER_SIZE 32u
#define ELF_ET_EXEC 2u
#define ELF_EM_386 3u
#define ELF_PT_LOAD 1u
#define ELF_PT_DYNAMIC 2u
#define ELF_PT_INTERP 3u
#define ELF_PF_X 1u
#define ELF_PF_W 2u
#define ELF_PF_R 4u
#define ELF_PAGE_SIZE 4096u

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int range_valid(size_t total, uint32_t offset, uint32_t length) {
    return (uint64_t)offset + (uint64_t)length <= (uint64_t)total;
}
static int power_of_two(uint32_t value) {
    return value && !(value & (value - 1u));
}
static int read_load_segment(const struct elf32_image *image, uint16_t index,
                             struct elf32_load_segment *segment) {
    if (!image || !image->bytes || !segment || image->size < ELF_HEADER_SIZE ||
        index >= image->program_header_count ||
        read16(image->bytes + 40) != ELF_HEADER_SIZE ||
        read16(image->bytes + 42) != ELF_PROGRAM_HEADER_SIZE ||
        !range_valid(image->size, read32(image->bytes + 28),
                     (uint32_t)image->program_header_count * ELF_PROGRAM_HEADER_SIZE))
        return -1;
    const uint8_t *ph = image->bytes + read32(image->bytes + 28) +
                        (size_t)index * ELF_PROGRAM_HEADER_SIZE;
    if (read32(ph) != ELF_PT_LOAD) return 1;
    segment->offset = read32(ph + 4);
    segment->virtual_address = read32(ph + 8);
    segment->file_size = read32(ph + 16);
    segment->memory_size = read32(ph + 20);
    segment->flags = read32(ph + 24);
    return 0;
}

int elf32_inspect(const void *data, size_t size, struct elf32_image *image) {
    if (!image) return -1;
    image->bytes = 0;
    image->size = 0;
    image->entry = 0;
    image->program_header_count = 0;
    if (!data || size < ELF_HEADER_SIZE) return -1;
    const uint8_t *b = (const uint8_t *)data;
    if (b[0] != 0x7F || b[1] != 'E' || b[2] != 'L' || b[3] != 'F' ||
        b[4] != 1 || b[5] != 1 || b[6] != 1 ||
        read16(b + 16) != ELF_ET_EXEC || read16(b + 18) != ELF_EM_386 ||
        read32(b + 20) != 1 || read16(b + 40) != ELF_HEADER_SIZE ||
        read16(b + 42) != ELF_PROGRAM_HEADER_SIZE)
        return -1;

    uint32_t phoff = read32(b + 28);
    uint16_t phnum = read16(b + 44);
    if (!phnum || phnum > ELF32_MAX_LOAD_SEGMENTS ||
        !range_valid(size, phoff, (uint32_t)phnum * ELF_PROGRAM_HEADER_SIZE))
        return -1;

    image->bytes = b;
    image->size = size;
    image->entry = read32(b + 24);
    image->program_header_count = phnum;

    uint32_t load_count = 0;
    int entry_executable = 0;
    for (uint16_t i = 0; i < phnum; ++i) {
        const uint8_t *ph = b + phoff + (size_t)i * ELF_PROGRAM_HEADER_SIZE;
        uint32_t type = read32(ph);
        if (type == ELF_PT_DYNAMIC || type == ELF_PT_INTERP) return -1;
        if (type != ELF_PT_LOAD) continue;

        struct elf32_load_segment seg;
        seg.offset = read32(ph + 4);
        seg.virtual_address = read32(ph + 8);
        seg.file_size = read32(ph + 16);
        seg.memory_size = read32(ph + 20);
        seg.flags = read32(ph + 24);
        uint32_t align = read32(ph + 28);

        if (!seg.memory_size || seg.file_size > seg.memory_size ||
            !range_valid(size, seg.offset, seg.file_size) ||
            seg.virtual_address < ELF32_USER_START ||
            (uint64_t)seg.virtual_address + seg.memory_size > ELF32_USER_STACK_PAGE ||
            (seg.virtual_address & (ELF_PAGE_SIZE - 1u)) ||
            (seg.offset & (ELF_PAGE_SIZE - 1u)) ||
            !(seg.flags & ELF_PF_R) ||
            (seg.flags & ~(ELF_PF_R | ELF_PF_W | ELF_PF_X)) ||
            ((seg.flags & ELF_PF_W) && (seg.flags & ELF_PF_X)) ||
            (align > 1u && (!power_of_two(align) ||
                            (seg.virtual_address % align) != (seg.offset % align))))
            return -1;

        uint64_t seg_end = (uint64_t)seg.virtual_address + seg.memory_size;
        for (uint16_t j = 0; j < i; ++j) {
            struct elf32_load_segment prior;
            int result = read_load_segment(image, j, &prior);
            if (result != 0) continue;
            uint64_t prior_end = (uint64_t)prior.virtual_address + prior.memory_size;
            if ((uint64_t)seg.virtual_address < prior_end &&
                (uint64_t)prior.virtual_address < seg_end)
                return -1;
        }
        if ((seg.flags & ELF_PF_X) && image->entry >= seg.virtual_address &&
            (uint64_t)image->entry < seg_end)
            entry_executable = 1;
        ++load_count;
    }
    return load_count && entry_executable ? 0 : -1;
}

int elf32_load_segment_at(const struct elf32_image *image, uint16_t index,
                          struct elf32_load_segment *segment) {
    if (!image || !image->bytes || !segment) return -1;
    return read_load_segment(image, index, segment);
}
