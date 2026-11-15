#ifndef BREADOS_SYSCALL_H
#define BREADOS_SYSCALL_H
#include <stdint.h>

/* Returns nonzero when the current user task has exited and must be switched out. */
int syscall_dispatch(uint32_t *saved_frame);
int syscall_abort_user_exception(uint32_t *saved_frame, uint32_t vector,
                                 uint32_t error_code);

#endif
