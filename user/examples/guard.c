#include "breados/syscall.h"

static volatile uint32_t user_state;

void _start(void) {
    static const char attempt[]=
        "BreadOS isolation test: attempting protected supervisor-memory write.\n";
    static const char failure[]=
        "FAIL: user mode modified supervisor memory.\n";

    user_state=1u;
    (void)bread_write(attempt,(uint32_t)(sizeof(attempt)-1u));

    volatile uint32_t *supervisor_memory=
        (volatile uint32_t *)(uintptr_t)0x00100000u;
    *supervisor_memory=0xBADC0DEu;

    (void)bread_write(failure,(uint32_t)(sizeof(failure)-1u));
    bread_exit(1);
}
