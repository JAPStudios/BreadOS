#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "heap.h"

int main(void) {
    heap_init();
    assert(heap_check_integrity());
    assert(heap_bytes_used()==0u);
    assert(heap_bytes_capacity()==256u*1024u);

    uint8_t *a=(uint8_t *)heap_alloc(1u);
    uint8_t *b=(uint8_t *)heap_alloc(17u);
    uint8_t *c=(uint8_t *)heap_alloc(4093u);
    assert(a && b && c);
    assert(((uintptr_t)a&15u)==0u);
    assert(((uintptr_t)b&15u)==0u);
    assert(((uintptr_t)c&15u)==0u);
    assert(a[0]==0xCDu && b[16]==0xCDu && c[4092]==0xCDu);
    assert(heap_check_integrity());

    a[0]=0x41u;
    b[16]=0x42u;
    c[4092]=0x43u;
    assert(heap_check_integrity());
    heap_free(b);
    assert(heap_check_integrity());
    heap_free(a);
    heap_free(c);
    assert(heap_check_integrity());
    assert(heap_bytes_used()==0u);

    uint32_t *zero=(uint32_t *)heap_calloc(8u,sizeof(uint32_t));
    assert(zero);
    for(size_t i=0;i<8u;i++) assert(zero[i]==0u);
    assert(heap_check_integrity());
    heap_free(zero);
    assert(heap_check_integrity());
    assert(heap_calloc((size_t)-1,2u)==0);
    assert(heap_alloc((size_t)-1)==0);
    heap_free((void *)(uintptr_t)0x1234u);
    assert(heap_check_integrity());

    puts("Hardened heap regression tests passed.");
    return 0;
}
