#include "heap.h"

#define ALIGNMENT 16u
#define CAPACITY 65536u

typedef struct Block {
    size_t size;
    unsigned free;
    struct Block *next;
} Block;

#define HEADER \
    ((sizeof(Block) + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1))

static unsigned char arena[CAPACITY]
    __attribute__((aligned(16)));

static Block *head;

void heap_init(void)
{
    head = (Block *)(void *)arena;
    head->size = CAPACITY - HEADER;
    head->free = 1;
    head->next = 0;
}

void *heap_alloc(size_t size)
{
    if (!head || !size || size > CAPACITY)
        return 0;

    size = (size + ALIGNMENT - 1)
        & ~(size_t)(ALIGNMENT - 1);

    for (Block *block = head; block; block = block->next) {
        if (!block->free || block->size < size)
            continue;

        if (block->size - size >= HEADER + ALIGNMENT) {
            Block *remaining = (Block *)(void *)(
                (unsigned char *)block + HEADER + size
            );

            remaining->size = block->size - size - HEADER;
            remaining->free = 1;
            remaining->next = block->next;

            block->next = remaining;
            block->size = size;
        }

        block->free = 0;
        return (unsigned char *)block + HEADER;
    }

    return 0;
}

int heap_free(void *pointer)
{
    if (!pointer)
        return 0;

    Block *found = 0;

    for (Block *block = head; block; block = block->next) {
        if ((unsigned char *)block + HEADER
                == (unsigned char *)pointer) {
            found = block;
            break;
        }
    }

    if (!found || found->free)
        return -1;

    found->free = 1;

    Block *block = head;

    while (block && block->next) {
        if (block->free && block->next->free) {
            block->size += HEADER + block->next->size;
            block->next = block->next->next;
        } else {
            block = block->next;
        }
    }

    return 0;
}

void *heap_calloc(size_t count, size_t size)
{
    if (!count || !size || count > (size_t)-1 / size)
        return 0;

    size_t total = count * size;
    unsigned char *memory = heap_alloc(total);

    if (memory) {
        for (size_t i = 0; i < total; ++i)
            memory[i] = 0;
    }

    return memory;
}

size_t heap_available(void)
{
    size_t total = 0;

    for (Block *block = head; block; block = block->next) {
        if (block->free)
            total += block->size;
    }

    return total;
}