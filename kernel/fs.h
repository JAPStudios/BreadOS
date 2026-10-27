#ifndef BREADOS_FS_H
#define BREADOS_FS_H

#include <stddef.h>
#include <stdint.h>

#define FS_BLOCK_SIZE 512u
#define FS_MAX_FILES 32u
#define FS_NAME_MAX 23u
#define FS_MAX_FILE_SIZE FS_BLOCK_SIZE

typedef int (*fs_block_read_fn)(void *context, uint32_t block, uint8_t out[FS_BLOCK_SIZE]);
typedef int (*fs_block_write_fn)(void *context, uint32_t block, const uint8_t data[FS_BLOCK_SIZE]);

typedef struct {
    void *context;
    uint32_t block_count;
    fs_block_read_fn read;
    fs_block_write_fn write;
} fs_block_device_t;

typedef struct {
    fs_block_device_t device;
    uint8_t mounted;
} fs_t;

/* The caller supplies a persistent block device. Blocks 0-2 hold metadata. */
int fs_format(const fs_block_device_t *device);
int fs_mount(fs_t *fs, const fs_block_device_t *device);
int fs_create(fs_t *fs, const char *name);
int fs_read(fs_t *fs, const char *name, void *out, size_t capacity, size_t *bytes_read);
int fs_write(fs_t *fs, const char *name, const void *data, size_t size);
int fs_move(fs_t *fs, const char *old_name, const char *new_name);
int fs_copy(fs_t *fs, const char *source_name, const char *destination_name);
int fs_delete(fs_t *fs, const char *name);

#endif
