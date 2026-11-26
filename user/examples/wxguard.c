#include "breados/syscall.h"

static volatile uint32_t writable_data;

void _start(void) {
    static const char attempt[]=
        "W^X test: attempting to execute writable data.\n";
    static const char limit_passed[]=
        "PASS: user CS limit excludes writable data.\n";
    static const char limit_failed[]=
        "FAIL: user CS limit includes writable data.\n";
    static const char limit_unavailable[]=
        "FAIL: user CS descriptor could not be inspected.\n";
    static const char failure[]=
        "FAIL: writable data executed as code.\n";

    uint16_t selector=0x1Bu;
    uint32_t code_limit=0;
    uint8_t limit_valid=0;
    __asm__ volatile (
        "lsl %2, %0\n\t"
        "setz %1"
        : "=r"(code_limit), "=qm"(limit_valid)
        : "rm"(selector)
        : "cc"
    );
    if (!limit_valid) {
        (void)bread_write(limit_unavailable,(uint32_t)(sizeof(limit_unavailable)-1u));
        bread_exit(2);
    }
    if (code_limit >= (uint32_t)(uintptr_t)&writable_data) {
        (void)bread_write(limit_failed,(uint32_t)(sizeof(limit_failed)-1u));
        bread_exit(3);
    }
    (void)bread_write(limit_passed,(uint32_t)(sizeof(limit_passed)-1u));
    (void)bread_write(attempt,(uint32_t)(sizeof(attempt)-1u));

    volatile uint8_t *code_bytes=(volatile uint8_t *)(uintptr_t)&writable_data;
    code_bytes[0]=0xC3u; /* ret */
    ((void (*)(void))(uintptr_t)&writable_data)();

    (void)bread_write(failure,(uint32_t)(sizeof(failure)-1u));
    bread_exit(1);
}
