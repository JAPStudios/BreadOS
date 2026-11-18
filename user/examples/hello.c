#include <stddef.h>
#include "breados/syscall.h"

static volatile uint32_t app_state;

__attribute__((used,noinline))
static void syscall_abi_compile_check(void) {
    char buffer[8]={0};
    uint32_t sender=0;
    (void)bread_write_char('x');
    (void)bread_send(1u,buffer,1u);
    (void)bread_receive(buffer,sizeof(buffer),&sender);
    (void)bread_task_id();
    int32_t descriptor=bread_open("/hello.txt",BREAD_O_READ);
    if(descriptor>=0) {
        (void)bread_read(descriptor,buffer,sizeof(buffer));
        (void)bread_file_write(descriptor,buffer,sizeof(buffer));
        (void)bread_seek(descriptor,0,0);
        (void)bread_close(descriptor);
    }
}

void _start(void) {
    app_state=1u;
    static const char message[]="BreadOS native user-mode application.\n";
    if (bread_write(message,(uint32_t)(sizeof(message)-1u))<0)
        bread_exit(1);
    bread_exit(0);
}
