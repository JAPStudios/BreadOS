#include "secure_memory.h"
void secure_memzero(void *memory,size_t length) {
    volatile unsigned char *bytes=(volatile unsigned char *)memory;
    if(!bytes) return;
    while(length--) *bytes++=0;
}
int secure_memeq(const void *left,const void *right,size_t length) {
    if((!left || !right) && length) return 0;
    const volatile unsigned char *a=(const volatile unsigned char *)left;
    const volatile unsigned char *b=(const volatile unsigned char *)right;
    unsigned char difference=0;
    for(size_t i=0;i<length;i++) difference|=(unsigned char)(a[i]^b[i]);
    return difference==0;
}
