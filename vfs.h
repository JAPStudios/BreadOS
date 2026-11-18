#ifndef BREADOS_VFS_H
#define BREADOS_VFS_H
#include <stddef.h>
#include <stdint.h>
#define VFS_PATH_MAX 63u
#define VFS_FILE_MAX 65535u
/* Maximum simultaneously open VFS descriptors per task. */
#define VFS_OPEN_MAX 4u
#define VFS_O_READ 0x01u
#define VFS_O_WRITE 0x02u
#define VFS_O_CREATE 0x04u
#define VFS_O_TRUNCATE 0x08u
#define VFS_O_BINARY 0x10u
enum vfs_node_type { VFS_NODE_FILE=1, VFS_NODE_DIRECTORY=2, VFS_NODE_DEVICE=3 };
enum vfs_backend { VFS_BACKEND_RAM=1, VFS_BACKEND_BREADFS=2, VFS_BACKEND_DEV=3, VFS_BACKEND_PROC=4 };
struct vfs_stat { enum vfs_node_type type; uint32_t size; };
struct vfs_dirent { char name[16]; enum vfs_node_type type; uint32_t size; };
void vfs_init(void);
int vfs_mount(const char *mount_path,enum vfs_backend backend);
int vfs_open(const char *path,uint32_t flags);
int vfs_close(int handle);
/* Discard every open handle owned by a task during deferred process cleanup. */
void vfs_close_task(uint32_t task_id);
int vfs_read(int handle,void *buffer,size_t count);
int vfs_write(int handle,const void *buffer,size_t count);
int vfs_seek(int handle,int32_t offset,int whence);
int vfs_stat_path(const char *path,struct vfs_stat *status);
int vfs_readdir(const char *path,size_t index,struct vfs_dirent *entry);
int vfs_mkdir(const char *path);
int vfs_unlink(const char *path);
int vfs_rmdir(const char *path);
int vfs_rename(const char *source,const char *destination);
int vfs_read_all(const char *path,void *buffer,size_t capacity,int binary);
int vfs_write_all(const char *path,const void *buffer,size_t size,int binary);
#endif
