#include "syscall.h"
#include "task.h"
#include "console.h"
#include <stdint.h>

struct syscall_frame {
    uint32_t edi, esi, ebp, ignored_esp, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};

int syscall_dispatch(uint32_t *saved_frame) {
    if (!saved_frame) return 0;
    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs&3u)!=3u) {
        frame->eax=0xFFFFFFFFu;
        return 0;
    }
    if (frame->eax==1u) {
        console_put_char((char)(frame->ebx&0xFFu));
        frame->eax=0;
        return 0;
    }
    if (frame->eax==2u) {
        return task_terminate_current((int)frame->ebx)==0;
    }
    frame->eax=0xFFFFFFFFu;
    return 0;
}

int syscall_abort_user_exception(uint32_t *saved_frame, uint32_t vector,
                                 uint32_t error_code) {
    (void)error_code;
    if (!saved_frame || vector >= 32u || vector == 2u ||
        vector == 8u || vector == 18u)
        return 0;

    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs & 3u) != 3u) return 0;
    return task_terminate_current(-(int)(vector+1u))==0;
}
