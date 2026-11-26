#include "breados/syscall.h"
static volatile uint32_t writable_data;
void _start(void) {
 static const char prefix[]="W^X task id: ";
 static const char write_ok[]="PASS: writable data write succeeded.\n";
 static const char attempt[]="W^X test: attempting to execute writable data.\n";
 static const char failure[]="FAIL: writable data executed as code.\n";
 uint32_t id=bread_task_id();
 (void)bread_write(prefix,(uint32_t)(sizeof(prefix)-1u));
 (void)bread_write_char((uint8_t)('0'+id));
 (void)bread_write_char('\n');
 writable_data=0xC3u;
 (void)bread_write(write_ok,(uint32_t)(sizeof(write_ok)-1u));
 (void)bread_write(attempt,(uint32_t)(sizeof(attempt)-1u));
 ((void (*)(void))(uintptr_t)&writable_data)();
 (void)bread_write(failure,(uint32_t)(sizeof(failure)-1u));
 bread_exit(1);
}
