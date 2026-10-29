#ifndef BREADOS_PFS_H
#define BREADOS_PFS_H
#include <stddef.h>

/*
 * BreadFS v1 uses 512-byte sectors and 16 metadata entries. Directory entries
 * use the existing reserved byte as a type flag, preserving the v1 disk layout.
 * Paths are limited to 15 characters; directories are listed with a trailing '/'.
 */
int pfs_init(void); /* 0: no usable ATA disk, 1: mounted existing volume, 2: formatted blank disk */
int pfs_ready(void);
size_t pfs_count(void);
const char *pfs_name(size_t index);
int pfs_read(const char *path, char *out, size_t capacity);
int pfs_write(const char *path, const char *data);
int pfs_remove(const char *path);
int pfs_mkdir(const char *path);
int pfs_rmdir(const char *path);
int pfs_move(const char *source, const char *destination);

#endif
