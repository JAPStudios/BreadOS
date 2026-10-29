#include "heap.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_CAPACITY (256u * 1024u)
#define HEAP_ALIGNMENT 16u
#define BLOCK_MAGIC 0xB4EAD05Au
#define BLOCK_FREE 1u
#define BLOCK_USED 2u

struct heap_block {
    size_t size;                 /* payload bytes, excluding this header */
    uint32_t magic;
    uint32_t state;
    struct heap_block *previous;
    struct heap_block *next;
};

static uint8_t arena[HEAP_CAPACITY] __attribute__((aligned(HEAP_ALIGNMENT)));
static struct heap_block *first;
static size_t used_bytes;
static int initialized;

static size_t align_up(size_t value) {
    return (value + HEAP_ALIGNMENT - 1u) & ~(size_t)(HEAP_ALIGNMENT - 1u);
}

static uint32_t lock_interrupts(void) {
    uint32_t flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static void unlock_interrupts(uint32_t flags) {
    if (flags & 0x200u)
        __asm__ volatile ("sti" : : : "memory");
}

void heap_init(void) {
    uint32_t flags = lock_interrupts();
    first = (struct heap_block *)(void *)arena;
    first->size = HEAP_CAPACITY - sizeof(*first);
    first->magic = BLOCK_MAGIC;
    first->state = BLOCK_FREE;
    first->previous = 0;
    first->next = 0;
    used_bytes = 0;
    initialized = 1;
    unlock_interrupts(flags);
}

static void split_block(struct heap_block *block, size_t requested) {
    if (block->size < requested + sizeof(struct heap_block) + HEAP_ALIGNMENT)
        return;

    uint8_t *payload = (uint8_t *)(void *)(block + 1);
    struct heap_block *remainder =
        (struct heap_block *)(void *)(payload + requested);
    remainder->size = block->size - requested - sizeof(*remainder);
    remainder->magic = BLOCK_MAGIC;
    remainder->state = BLOCK_FREE;
    remainder->previous = block;
    remainder->next = block->next;
    if (remainder->next)
        remainder->next->previous = remainder;
    block->next = remainder;
    block->size = requested;
}

void *heap_alloc(size_t size) {
    if (!initialized || size == 0 || size > HEAP_CAPACITY - sizeof(struct heap_block))
        return 0;

    size = align_up(size);
    if (size < align_up(size)) return 0;

    uint32_t flags = lock_interrupts();
    for (struct heap_block *block = first; block; block = block->next) {
        if (block->magic != BLOCK_MAGIC) {
            unlock_interrupts(flags);
            return 0;
        }
        if (block->state == BLOCK_FREE && block->size >= size) {
            split_block(block, size);
            block->state = BLOCK_USED;
            used_bytes += block->size;
            void *result = (void *)(block + 1);
            unlock_interrupts(flags);
            return result;
        }
    }
    unlock_interrupts(flags);
    return 0;
}

void *heap_calloc(size_t count, size_t size) {
    if (size && count > (size_t)-1 / size) return 0;
    size_t total = count * size;
    void *pointer = heap_alloc(total);
    if (!pointer) return 0;
    uint8_t *bytes = (uint8_t *)pointer;
    for (size_t i = 0; i < total; ++i) bytes[i] = 0;
    return pointer;
}

void heap_free(void *pointer) {
    if (!pointer || !initialized) return;

    uintptr_t address = (uintptr_t)pointer;
    uintptr_t start = (uintptr_t)arena + sizeof(struct heap_block);
    uintptr_t end = (uintptr_t)arena + sizeof(arena);
    if (address < start || address >= end ||
        ((address - (uintptr_t)arena - sizeof(struct heap_block)) % HEAP_ALIGNMENT) != 0)
        return;

    uint32_t flags = lock_interrupts();
    struct heap_block *block = (struct heap_block *)pointer - 1;
    if (block->magic != BLOCK_MAGIC || block->state != BLOCK_USED ||
        (uint8_t *)(void *)block < arena ||
        (uint8_t *)(void *)block >= arena + sizeof(arena)) {
        unlock_interrupts(flags);
        return;
    }

    block->state = BLOCK_FREE;
    if (used_bytes >= block->size) used_bytes -= block->size;
    else used_bytes = 0;

    if (block->next && block->next->magic == BLOCK_MAGIC &&
        block->next->state == BLOCK_FREE) {
        struct heap_block *next = block->next;
        block->size += sizeof(*next) + next->size;
        block->next = next->next;
        if (block->next) block->next->previous = block;
    }
    if (block->previous && block->previous->magic == BLOCK_MAGIC &&
        block->previous->state == BLOCK_FREE) {
        struct heap_block *previous = block->previous;
        previous->size += sizeof(*block) + block->size;
        previous->next = block->next;
        if (previous->next) previous->next->previous = previous;
    }
    unlock_interrupts(flags);
}

size_t heap_bytes_used(void) {
    return used_bytes;
}

size_t heap_bytes_capacity(void) {
    return HEAP_CAPACITY;
}
