#ifndef BREADOS_PROCESS_H
#define BREADOS_PROCESS_H
#include <stddef.h>
#include <stdint.h>

/* Start an ELF32/i386 program as an independently scheduled ring-3 task. */
int process_run_user_demo(void);
int process_execute_elf(const void *image, size_t image_size);
int process_execute_file(const char *path);

/* Reclaim exited processes only after the scheduler has switched away from them. */
void process_reap_zombies(void);
uint32_t process_active_count(void);
uint32_t process_snapshot(uint32_t *task_ids,uint8_t *exiting,uint32_t capacity);

#endif
