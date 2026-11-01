#ifndef BREADOS_HEAP_H
#define BREADOS_HEAP_H

#include <stddef.h>
#include <stdint.h>

/* Kernel heap: 256 KiB static arena, 16-byte aligned allocations.
 * Allocations have an address/size-bound trailing canary; block headers have
 * integrity guards; freed payloads are poisoned. Initialize once during boot.
 * This remains a single-core allocator; SMP locking is not implemented.
 */
void heap_init(void);
void *heap_alloc(size_t size);
void *heap_calloc(size_t count, size_t size);
void heap_free(void *pointer);
size_t heap_bytes_used(void);
size_t heap_bytes_capacity(void);
/* Returns 1 only when the complete block chain and all live canaries validate. */
int heap_check_integrity(void);

#endif
