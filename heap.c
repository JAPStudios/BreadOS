#include "heap.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_CAPACITY (256u * 1024u)
#define HEAP_ALIGNMENT 16u
#define BLOCK_MAGIC 0xB4EAD05Au
#define BLOCK_GUARD 0x91E10DA5u
#define BLOCK_FREE 1u
#define BLOCK_USED 2u
#define ALLOCATION_CANARY 0xC0FFEE11u
#define FREE_POISON 0xDDu
#define ALLOC_POISON 0xCDu
#define CANARY_SIZE sizeof(uint32_t)
#define HEAP_HEADER_SIZE ((sizeof(struct heap_block) + HEAP_ALIGNMENT - 1u) & ~(size_t)(HEAP_ALIGNMENT - 1u))

struct heap_block {
    size_t size;
    size_t requested_size;
    uint32_t magic;
    uint32_t state;
    uint32_t guard;
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
#if defined(__i386__)
    uint32_t flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
#else
    return 0;
#endif
}

static void unlock_interrupts(uint32_t flags) {
#if defined(__i386__)
    if (flags & 0x200u)
        __asm__ volatile ("sti" : : : "memory");
#else
    (void)flags;
#endif
}

static int block_in_arena(const struct heap_block *block) {
    uintptr_t address=(uintptr_t)block;
    uintptr_t start=(uintptr_t)arena;
    uintptr_t end=start+sizeof(arena);
    return address>=start && address<=end-HEAP_HEADER_SIZE &&
           ((address-start)%HEAP_ALIGNMENT)==0;
}

static uint8_t *payload_of(struct heap_block *block) {
    return (uint8_t *)(void *)block + HEAP_HEADER_SIZE;
}

static uint32_t block_guard(const struct heap_block *block) {
    return BLOCK_GUARD ^ (uint32_t)(uintptr_t)block ^
           (uint32_t)block->size ^ (uint32_t)block->requested_size ^
           block->magic;
}

static int block_valid(const struct heap_block *block) {
    return block_in_arena(block) && block->magic == BLOCK_MAGIC &&
           block->guard == block_guard(block) &&
           (block->state == BLOCK_FREE || block->state == BLOCK_USED);
}

static void write_u32(uint8_t *destination, uint32_t value) {
    destination[0]=(uint8_t)value;
    destination[1]=(uint8_t)(value>>8);
    destination[2]=(uint8_t)(value>>16);
    destination[3]=(uint8_t)(value>>24);
}

static uint32_t read_u32(const uint8_t *source) {
    return (uint32_t)source[0] | ((uint32_t)source[1]<<8) |
           ((uint32_t)source[2]<<16) | ((uint32_t)source[3]<<24);
}

static uint32_t allocation_canary(const struct heap_block *block) {
    return ALLOCATION_CANARY ^ (uint32_t)(uintptr_t)payload_of((struct heap_block *)block) ^
           (uint32_t)block->requested_size;
}

static int allocation_valid(const struct heap_block *block) {
    if (!block_valid(block) || block->state != BLOCK_USED ||
        block->requested_size > block->size ||
        block->requested_size + CANARY_SIZE > block->size)
        return 0;
    return read_u32(payload_of((struct heap_block *)block)+block->requested_size) ==
           allocation_canary(block);
}

static void refresh_guard(struct heap_block *block) {
    block->guard=block_guard(block);
}

void heap_init(void) {
    uint32_t flags = lock_interrupts();
    first = (struct heap_block *)(void *)arena;
    first->size = HEAP_CAPACITY - HEAP_HEADER_SIZE;
    first->requested_size = 0;
    first->magic = BLOCK_MAGIC;
    first->state = BLOCK_FREE;
    first->previous = 0;
    first->next = 0;
    refresh_guard(first);
    used_bytes = 0;
    initialized = 1;
    unlock_interrupts(flags);
}

static void split_block(struct heap_block *block, size_t requested_capacity) {
    if (block->size < requested_capacity + HEAP_HEADER_SIZE + HEAP_ALIGNMENT)
        return;

    uint8_t *payload = payload_of(block);
    struct heap_block *remainder =
        (struct heap_block *)(void *)(payload + requested_capacity);
    remainder->size = block->size - requested_capacity - HEAP_HEADER_SIZE;
    remainder->requested_size = 0;
    remainder->magic = BLOCK_MAGIC;
    remainder->state = BLOCK_FREE;
    remainder->previous = block;
    remainder->next = block->next;
    refresh_guard(remainder);
    if (remainder->next && block_valid(remainder->next))
        remainder->next->previous = remainder;
    block->next = remainder;
    block->size = requested_capacity;
    refresh_guard(block);
}

void *heap_alloc(size_t size) {
    if (!initialized || size == 0 || size > HEAP_CAPACITY - HEAP_HEADER_SIZE ||
        size > (size_t)-1 - CANARY_SIZE - (HEAP_ALIGNMENT - 1u))
        return 0;

    size_t requested_size=size;
    size_t capacity=align_up(size+CANARY_SIZE);
    uint32_t flags = lock_interrupts();
    for (struct heap_block *block = first; block; block = block->next) {
        if (!block_valid(block)) {
            unlock_interrupts(flags);
            return 0;
        }
        if (block->state == BLOCK_FREE && block->size >= capacity) {
            split_block(block, capacity);
            block->state = BLOCK_USED;
            block->requested_size=requested_size;
            refresh_guard(block);
            uint8_t *payload=payload_of(block);
            for(size_t i=0;i<requested_size;i++) payload[i]=ALLOC_POISON;
            write_u32(payload+requested_size,allocation_canary(block));
            used_bytes += block->size;
            unlock_interrupts(flags);
            return payload;
        }
    }
    unlock_interrupts(flags);
    return 0;
}

void *heap_calloc(size_t count, size_t size) {
    if (size && count > (size_t)-1 / size) return 0;
    size_t total = count * size;
    if (!total) return 0;
    void *pointer = heap_alloc(total);
    if (!pointer) return 0;
    uint8_t *bytes = (uint8_t *)pointer;
    for (size_t i = 0; i < total; ++i) bytes[i] = 0;
    return pointer;
}

void heap_free(void *pointer) {
    if (!pointer || !initialized) return;

    uintptr_t address = (uintptr_t)pointer;
    uintptr_t start = (uintptr_t)arena + HEAP_HEADER_SIZE;
    uintptr_t end = (uintptr_t)arena + sizeof(arena);
    if (address < start || address >= end ||
        ((address - (uintptr_t)arena - HEAP_HEADER_SIZE) % HEAP_ALIGNMENT) != 0)
        return;

    uint32_t flags = lock_interrupts();
    struct heap_block *block = (struct heap_block *)(void *)((uint8_t *)pointer - HEAP_HEADER_SIZE);
    if (!allocation_valid(block)) {
        unlock_interrupts(flags);
        return;
    }

    if (used_bytes >= block->size) used_bytes -= block->size;
    else used_bytes = 0;
    for(size_t i=0;i<block->size;i++) ((uint8_t *)pointer)[i]=FREE_POISON;
    block->state = BLOCK_FREE;
    block->requested_size=0;
    refresh_guard(block);

    if (block->next) {
        if (!block_valid(block->next)) { unlock_interrupts(flags); return; }
        if (block->next->state == BLOCK_FREE) {
            struct heap_block *next = block->next;
            block->size += HEAP_HEADER_SIZE + next->size;
            block->next = next->next;
            if (block->next && block_valid(block->next))
                block->next->previous = block;
            refresh_guard(block);
        }
    }
    if (block->previous) {
        if (!block_valid(block->previous)) { unlock_interrupts(flags); return; }
        if (block->previous->state == BLOCK_FREE) {
            struct heap_block *previous = block->previous;
            previous->size += HEAP_HEADER_SIZE + block->size;
            previous->next = block->next;
            if (previous->next && block_valid(previous->next))
                previous->next->previous = previous;
            refresh_guard(previous);
        }
    }
    unlock_interrupts(flags);
}

int heap_check_integrity(void) {
    if(!initialized) return 0;
    uint32_t flags=lock_interrupts();
    uintptr_t arena_start=(uintptr_t)arena;
    uintptr_t arena_end=arena_start+sizeof(arena);
    struct heap_block *previous=0;
    size_t accounted=0;
    size_t traversed=0;
    for(struct heap_block *block=first;block;) {
        if(!block_valid(block) || block->previous!=previous ||
           block->size>arena_end-(uintptr_t)block-HEAP_HEADER_SIZE ||
           ++traversed>HEAP_CAPACITY/HEAP_HEADER_SIZE) {
            unlock_interrupts(flags);
            return 0;
        }
        if(block->state==BLOCK_USED) {
            if(!allocation_valid(block)) { unlock_interrupts(flags); return 0; }
            accounted+=block->size;
        } else if(block->requested_size!=0) {
            unlock_interrupts(flags);
            return 0;
        }
        previous=block;
        block=block->next;
    }
    int valid=accounted==used_bytes;
    unlock_interrupts(flags);
    return valid;
}

size_t heap_bytes_used(void) { return used_bytes; }
size_t heap_bytes_capacity(void) { return HEAP_CAPACITY; }
