#ifndef BREADOS_ELF_LOADER_H
#define BREADOS_ELF_LOADER_H

#include <stddef.h>
#include <stdint.h>

#define ELF32_MAX_LOAD_SEGMENTS 32u
#define ELF32_USER_START 0x40000000u
#define ELF32_USER_END   0x40400000u
#define ELF32_USER_STACK_PAGE (ELF32_USER_END - 4096u)
/* Keep one unmapped page immediately below the user stack as a guard. */
#define ELF32_USER_STACK_GUARD_PAGE (ELF32_USER_STACK_PAGE - 4096u)

struct elf32_image {
    const uint8_t *bytes;
    size_t size;
    uint32_t entry;
    uint16_t program_header_count;
};

struct elf32_load_segment {
    uint32_t offset;
    uint32_t virtual_address;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
};

/* Validate a complete, static, little-endian i386 ELF executable. */
int elf32_inspect(const void *bytes, size_t size, struct elf32_image *image);

/* Return the indexed PT_LOAD segment after successful inspection. */
int elf32_load_segment_at(const struct elf32_image *image, uint16_t index,
                          struct elf32_load_segment *segment);

/* Inclusive i386 user CS limit derived from executable PT_LOAD segments. */
int elf32_user_code_limit(const struct elf32_image *image,
                          uint32_t *inclusive_limit);

#endif
