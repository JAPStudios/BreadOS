#ifndef BREADOS_FS_H
#define BREADOS_FS_H
#include <stddef.h>
#include <stdint.h>

/* Volatile RAM filesystem: contents are lost when the machine reboots. */
#define FS_MAX_FILES 16u
#define FS_NAME_MAX  15u
#define FS_DATA_MAX  255u

void fs_init(void);
size_t fs_count(void);
const char *fs_name(size_t index);
int fs_read(const char *name, char *out, size_t capacity);
int fs_write(const char *name, const char *data);
int fs_remove(const char *name);
#endif
