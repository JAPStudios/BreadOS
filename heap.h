#ifndef HEAP_H
#define HEAP_H
#include <stddef.h>
void heap_init(void);
void *heap_alloc(size_t size);
void *heap_calloc(size_t count,size_t size);
int heap_free(void *pointer);
size_t heap_available(void);
#endif
