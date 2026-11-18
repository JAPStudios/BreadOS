#include "breados/syscall.h"

static volatile uint32_t writable_data;

void _start(void) {
    static const char attempt[]=
        "W^X test: attempting to execute writable data.\n";
    static const char failure[]=
        "FAIL: writable data executed as code.\n";

    (void)bread_write(attempt,(uint32_t)(sizeof(attempt)-1u));
    volatile uint8_t *code_bytes=(volatile uint8_t *)(uintptr_t)&writable_data;
    code_bytes[0]=0xC3u; /* ret */
    ((void (*)(void))(uintptr_t)&writable_data)();

    (void)bread_write(failure,(uint32_t)(sizeof(failure)-1u));
    bread_exit(1);
}
