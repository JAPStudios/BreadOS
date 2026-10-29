#ifndef BREADOS_HEAP_H
#define BREADOS_HEAP_H

#include <stddef.h>
#include <stdint.h>

/* Kernel heap: 256 KiB static arena, 16-byte aligned allocations.
 * Initialize once during boot before calling heap_alloc/heap_free.
 * This is a single-core kernel allocator; SMP locking is not implemented.
 */
void heap_init(void);
void *heap_alloc(size_t size);
void *heap_calloc(size_t count, size_t size);
void heap_free(void *pointer);
size_t heap_bytes_used(void);
size_t heap_bytes_capacity(void);

#endif
