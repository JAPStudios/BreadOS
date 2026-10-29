#ifndef BREADOS_SYSCALL_H
#define BREADOS_SYSCALL_H

#include <stdint.h>

/* Saved by interrupts.s: pusha registers followed by vector/error and iret frame. */
struct syscall_frame {
    uint32_t edi, esi, ebp, saved_esp, ebx, edx, ecx, eax;
    uint32_t vector, error_code, eip, cs, eflags;
    /* User ESP/SS are present only when the interrupt originated in ring 3. */
    uint32_t user_esp, user_ss;
};

/* Configure the syscall gate before enabling interrupts. */
void syscall_init(void);

/* Dispatches a syscall from the saved interrupt frame. */
void syscall_dispatch(struct syscall_frame *frame);

#endif
