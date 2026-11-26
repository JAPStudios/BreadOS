#include "breados/syscall.h"

static volatile uint32_t writable_data;

void _start(void) {
    static const char task_prefix[]="W^X task id: ";
    static const char selector_ok[]="PASS: W^X task loaded its private LDT code selector.\n";
    static const char limit_ok[]="PASS: user CS limit excludes writable data.\n";
    static const char attempt[]="W^X test: attempting to execute writable data.\n";
    static const char bad_task[]="FAIL: W^X test did not run in task slot 4.\n";
    static const char bad_selector[]="FAIL: W^X test loaded the wrong code selector.\n";
    static const char bad_limit[]="FAIL: user CS limit includes writable data.\n";
    static const char no_descriptor[]="FAIL: user CS descriptor could not be inspected.\n";
    static const char failure[]="FAIL: writable data executed as code.\n";

    uint32_t task_id=bread_task_id();
    (void)bread_write(task_prefix,(uint32_t)(sizeof(task_prefix)-1u));
    (void)bread_write_char((uint8_t)('0'+task_id));
    (void)bread_write_char('\n');
    if (task_id!=4u) {
        (void)bread_write(bad_task,(uint32_t)(sizeof(bad_task)-1u));
        bread_exit(2);
    }

    uint16_t selector=0;
    uint32_t code_limit=0;
    uint8_t limit_valid=0;
    __asm__ volatile ("movw %%cs, %0" : "=rm"(selector));
    if (selector!=0x0Fu) {
        (void)bread_write(bad_selector,(uint32_t)(sizeof(bad_selector)-1u));
        bread_exit(3);
    }
    __asm__ volatile (
        "lsl %2, %0\n\t"
        "setz %1"
        : "=r"(code_limit), "=qm"(limit_valid)
        : "rm"(selector)
        : "cc"
    );
    if (!limit_valid) {
        (void)bread_write(no_descriptor,(uint32_t)(sizeof(no_descriptor)-1u));
        bread_exit(4);
    }
    if (code_limit >= (uint32_t)(uintptr_t)&writable_data) {
        (void)bread_write(bad_limit,(uint32_t)(sizeof(bad_limit)-1u));
        bread_exit(5);
    }

    (void)bread_write(selector_ok,(uint32_t)(sizeof(selector_ok)-1u));
    (void)bread_write(limit_ok,(uint32_t)(sizeof(limit_ok)-1u));
    (void)bread_write(attempt,(uint32_t)(sizeof(attempt)-1u));
    volatile uint8_t *code_bytes=(volatile uint8_t *)(uintptr_t)&writable_data;
    code_bytes[0]=0xC3u; /* ret */
    ((void (*)(void))(uintptr_t)&writable_data)();

    (void)bread_write(failure,(uint32_t)(sizeof(failure)-1u));
    bread_exit(1);
}
