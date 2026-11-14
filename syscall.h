#ifndef BREADOS_SYSCALL_H
#define BREADOS_SYSCALL_H
#include <stdint.h>

/* Dispatches the int 0x80 frame; called only from a ring-3 gate. */
void syscall_dispatch(uint32_t *saved_frame);

/* Convert recoverable ring-3 exceptions into process termination. */
int syscall_abort_user_exception(uint32_t *saved_frame, uint32_t vector,
                                 uint32_t error_code);
int syscall_last_exit_code(void);

#endif
