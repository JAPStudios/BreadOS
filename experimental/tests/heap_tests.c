#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "heap.h"
int main(void){
 assert(heap_alloc(16)==0);heap_init();size_t initial=heap_available();
 assert(initial>64000);assert(heap_alloc(0)==0);
 void *a=heap_alloc(1),*b=heap_alloc(100),*c=heap_alloc(500);
 assert(a&&b&&c);assert(((uintptr_t)a&15)==0);assert(((uintptr_t)b&15)==0);
 assert(heap_free((char *)b+1)==-1);
 int other=0;assert(heap_free(&other)==-1);
 assert(heap_free(b)==0);assert(heap_free(b)==-1);
 assert(heap_free(a)==0);assert(heap_free(c)==0);assert(heap_available()==initial);
 unsigned char *zero=heap_calloc(32,4);assert(zero);
 for(unsigned i=0;i<128;i++)assert(zero[i]==0);
 assert(heap_free(zero)==0);
 assert(heap_calloc((size_t)-1,2)==0);
 assert(heap_alloc((size_t)-1)==0);
 void *items[4096];unsigned n=0;
 while(n<4096&&(items[n]=heap_alloc(16)))n++;
 assert(n>0&&n<4096);assert(heap_alloc(16)==0);
 for(unsigned i=0;i<n;i++)assert(heap_free(items[i])==0);
 assert(heap_available()==initial);assert(heap_free(0)==0);
 puts("PASS: initialization, alignment, allocation, free, invalid/double-free rejection, zeroed allocation, overflow, exhaustion and coalescing.");
}
