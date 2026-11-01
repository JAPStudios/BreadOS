#include "syscall.h"
#include "video.h"
#include "console.h"
#include <stdint.h>

#define KERNEL_CODE_SELECTOR 0x08u

struct syscall_frame {
    uint32_t edi, esi, ebp, ignored_esp, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};

extern void user_exit_trampoline(void);
static int last_exit_code;

void syscall_dispatch(uint32_t *saved_frame) {
    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs&3u)!=3u) {
        frame->eax=0xFFFFFFFFu;
        return;
    }
    if (frame->eax==1u) {
        console_put_char((char)(frame->ebx&0xFFu));
        frame->eax=0;
        return;
    }
    if (frame->eax==2u) {
        last_exit_code=(int)frame->ebx;
        frame->eax=(uint32_t)last_exit_code;
        frame->eip=(uint32_t)(uintptr_t)user_exit_trampoline;
        frame->cs=KERNEL_CODE_SELECTOR;
        frame->eflags=0x202u;
        return;
    }
    frame->eax=0xFFFFFFFFu;
}

int syscall_last_exit_code(void) { return last_exit_code; }
