#ifndef BREADOS_PFS_H
#define BREADOS_PFS_H
#include <stddef.h>

/*
 * BreadFS v1 is a small, single-disk filesystem for development/testing.
 * It stores 16 named files, each up to 255 bytes, in fixed 512-byte sectors.
 */
int pfs_init(void); /* 0: no usable ATA disk, 1: mounted existing volume, 2: formatted blank disk */
int pfs_ready(void);
size_t pfs_count(void);
const char *pfs_name(size_t index);
int pfs_read(const char *name, char *out, size_t capacity);
int pfs_write(const char *name, const char *data);
int pfs_remove(const char *name);

#endif
