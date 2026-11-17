#include <assert.h>
#include <stdint.h>
#include "syscall.h"
#include "task.h"
#include "paging.h"
#include "ipc.h"
#include "console.h"
#include "vfs.h"

struct test_frame {
    uint32_t edi, esi, ebp, ignored_esp, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};

static uint32_t current_id = 7;
static int terminate_result;
static int terminate_calls;
static char console_char;
static int console_calls;

uint32_t task_current_id(void) { return current_id; }
const struct paging_address_space *task_current_address_space(void) { return 0; }
int task_terminate_current(int code) {
    (void)code;
    terminate_calls++;
    return terminate_result;
}
int paging_user_range_valid(const struct paging_address_space *space,
                            uint32_t address, uint32_t length, int writable) {
    (void)space; (void)address; (void)length; (void)writable;
    return 0;
}
void console_put_char(char c) { console_char=c; console_calls++; }
int ipc_send(uint32_t sender,uint32_t target,const void *data,uint32_t length) {
    (void)sender; (void)target; (void)data; (void)length; return -1;
}
int ipc_receive(uint32_t receiver,void *data,uint32_t capacity,uint32_t *sender) {
    (void)receiver; (void)data; (void)capacity; (void)sender; return -1;
}
int vfs_open(const char *path,uint32_t flags) { (void)path; (void)flags; return -1; }
int vfs_close(int handle) { (void)handle; return -1; }
int vfs_read(int handle,void *buffer,size_t count) {
    (void)handle; (void)buffer; (void)count; return -1;
}
int vfs_write(int handle,const void *buffer,size_t count) {
    (void)handle; (void)buffer; (void)count; return -1;
}
int vfs_seek(int handle,int32_t offset,int whence) {
    (void)handle; (void)offset; (void)whence; return -1;
}

static struct test_frame frame_for(uint32_t syscall_number) {
    struct test_frame frame={0};
    frame.eax=syscall_number;
    frame.cs=0x1Bu;
    return frame;
}

int main(void) {
    struct test_frame frame=frame_for(5u);
    assert(syscall_dispatch((uint32_t *)&frame)==0);
    assert(frame.eax==current_id);

    frame=frame_for(1u);
    frame.ebx='B';
    assert(syscall_dispatch((uint32_t *)&frame)==0);
    assert(frame.eax==0 && console_calls==1 && console_char=='B');

    frame=frame_for(0xFFFFFFFEu);
    assert(syscall_dispatch((uint32_t *)&frame)==0);
    assert(frame.eax==0xFFFFFFFFu);

    frame=frame_for(5u);
    frame.cs=0x08u;
    assert(syscall_dispatch((uint32_t *)&frame)==0);
    assert(frame.eax==0xFFFFFFFFu);

    terminate_result=0;
    frame=frame_for(2u);
    frame.ebx=23u;
    assert(syscall_dispatch((uint32_t *)&frame)==1);
    assert(terminate_calls==1);

    terminate_result=-1;
    frame=frame_for(2u);
    assert(syscall_dispatch((uint32_t *)&frame)==0);
    assert(terminate_calls==2);

    terminate_result=0;
    frame=frame_for(5u);
    assert(syscall_abort_user_exception((uint32_t *)&frame,13u,0u)==1);
    assert(syscall_abort_user_exception((uint32_t *)&frame,2u,0u)==0);
    assert(syscall_abort_user_exception((uint32_t *)&frame,8u,0u)==0);
    assert(syscall_abort_user_exception((uint32_t *)&frame,18u,0u)==0);
    frame.cs=0x08u;
    assert(syscall_abort_user_exception((uint32_t *)&frame,13u,0u)==0);
    return 0;
}
