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

int syscall_abort_user_exception(uint32_t *saved_frame, uint32_t vector,
                                 uint32_t error_code) {
    (void)error_code;
    if (!saved_frame || vector >= 32u || vector == 2u ||
        vector == 8u || vector == 18u)
        return 0;

    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs & 3u) != 3u) return 0;

    /* Keep ordinary user faults from taking down the kernel. Double faults,
       NMIs and machine checks remain fatal because their recovery is unsafe. */
    last_exit_code=-(int)(vector+1u);
    frame->eax=(uint32_t)last_exit_code;
    frame->eip=(uint32_t)(uintptr_t)user_exit_trampoline;
    frame->cs=KERNEL_CODE_SELECTOR;
    frame->eflags=0x202u;
    return 1;
}

void syscall_begin_process(void) { last_exit_code = -1; }

int syscall_last_exit_code(void) { return last_exit_code; }
