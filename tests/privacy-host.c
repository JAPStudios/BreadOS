#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "privacy.h"

int main(void) {
    char encoded[16];
    enum bread_privacy_capability capability = BREAD_PRIVACY_CAMERA;
    privacy_init();
    for (unsigned i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++)
        assert(!privacy_allows((enum bread_privacy_capability)i));
    assert(!privacy_allows((enum bread_privacy_capability)99));
    assert(privacy_find("MICROPHONE",&capability)==0);
    assert(capability==BREAD_PRIVACY_MICROPHONE);
    assert(privacy_set(capability,1)==0);
    assert(privacy_set(BREAD_PRIVACY_NETWORK,1)==0);
    assert(privacy_set(BREAD_PRIVACY_CAMERA,1)==0);
    assert(privacy_set(BREAD_PRIVACY_CAMERA,2)==-1);
    assert(privacy_set((enum bread_privacy_capability)99,1)==-1);
    assert(privacy_serialize(encoded,sizeof(encoded))==10);
    assert(strcmp(encoded,"P1:1101000")==0);
    privacy_reset();
    assert(privacy_deserialize(encoded)==0);
    assert(privacy_allows(BREAD_PRIVACY_CAMERA));
    assert(privacy_allows(BREAD_PRIVACY_MICROPHONE));
    assert(privacy_allows(BREAD_PRIVACY_NETWORK));
    assert(!privacy_allows(BREAD_PRIVACY_LOCATION));
    assert(privacy_deserialize("P1:10010x0")==-1);
    assert(privacy_allows(BREAD_PRIVACY_CAMERA));
    assert(privacy_deserialize("P2:1001000")==-1);
    assert(privacy_deserialize("P1:10010000")==-1);
    assert(privacy_serialize(encoded,4)==-1);
    privacy_reset();
    assert(!privacy_allows(BREAD_PRIVACY_CAMERA));
    puts("privacy policy tests passed");
    return 0;
}
