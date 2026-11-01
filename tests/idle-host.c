#include <assert.h>
#include <stdio.h>
#include "idle.h"

int main(void) {
    idle_set_timeout(300u);
    idle_init(1000u);
    assert(!idle_poll(1299u));
    assert(idle_poll(1300u));
    assert(idle_is_active());
    idle_activity(1301u);
    assert(!idle_is_active());
    assert(!idle_poll(1500u));
    idle_set_timeout(0);
    assert(!idle_poll(9000u));
    idle_set_timeout(100u);
    idle_init(0xFFFFFFF0u);
    assert(!idle_poll(0x20u));
    assert(idle_poll(0x54u));
    puts("Idle manager regression tests passed.");
    return 0;
}
