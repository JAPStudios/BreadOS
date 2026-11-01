#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "secure_memory.h"
int main(void) {
    unsigned char a[8]={1,2,3,4,5,6,7,8};
    unsigned char b[8]={1,2,3,4,5,6,7,8};
    assert(secure_memeq(a,b,sizeof(a)));
    b[7]=9;
    assert(!secure_memeq(a,b,sizeof(a)));
    assert(secure_memeq(a,b,0));
    assert(!secure_memeq(0,b,1));
    secure_memzero(a,sizeof(a));
    for(size_t i=0;i<sizeof(a);i++) assert(a[i]==0);
    secure_memzero(0,8);
    puts("secure memory tests passed");
    return 0;
}
