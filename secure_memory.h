#ifndef BREAD_SECURE_MEMORY_H
#define BREAD_SECURE_MEMORY_H
#include <stddef.h>
void secure_memzero(void *memory,size_t length);
int secure_memeq(const void *left,const void *right,size_t length);
#endif
