#ifndef BREADOS_PROCESS_H
#define BREADOS_PROCESS_H
#include <stddef.h>
/* Runs the isolated ring-3 syscall smoke program and returns its exit code. */
int process_run_user_demo(void);

/* Load and run a validated static ELF32/i386 image in an isolated address space. */
int process_execute_elf(const void *image, size_t image_size);
int process_execute_file(const char *path);
#endif
