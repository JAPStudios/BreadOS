#ifndef BREADOS_SYSCALL_H
#define BREADOS_SYSCALL_H
#include <stdint.h>

/* Dispatches the int 0x80 frame; called only from a ring-3 gate. */
void syscall_dispatch(uint32_t *saved_frame);
int syscall_last_exit_code(void);

#endif
