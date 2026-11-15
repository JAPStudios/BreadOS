#include "vfs.h"
#include "fs.h"
#include "pfs.h"
#include "console.h"
#include "memory.h"
#include "interrupts.h"
#include "process.h"
#include "task.h"
#include "kernel/keyboard.h"
#include <stdint.h>

#define VFS_MOUNT_MAX 4u
#define VFS_NAME_MAX FS_NAME_MAX

struct vfs_mount_entry {
    uint8_t used;
    enum vfs_backend backend;
    char path[VFS_PATH_MAX+1u];
};

struct vfs_handle {
    uint8_t used;
    uint8_t dirty;
    uint8_t device;
    uint8_t reserved;
    uint32_t flags;
    enum vfs_backend backend;
    uint32_t position;
    uint32_t size;
    char path[VFS_PATH_MAX+1u];
    uint8_t data[VFS_FILE_MAX];
};

static struct vfs_mount_entry mounts[VFS_MOUNT_MAX];
static struct vfs_handle handles[VFS_OPEN_MAX];

static size_t str_len(const char *s) {
    size_t n=0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}
static int str_eq(const char *a,const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a==*b) { ++a; ++b; }
    return *a==*b;
}
static int str_starts(const char *s,const char *prefix) {
    while (*prefix) { if (*s++!=*prefix++) return 0; }
    return 1;
}
static void bytes_copy(void *destination,const void *source,size_t count) {
    uint8_t *d=(uint8_t *)destination;
    const uint8_t *s=(const uint8_t *)source;
    for (size_t i=0;i<count;i++) d[i]=s[i];
}
static void bytes_zero(void *destination,size_t count) {
    volatile uint8_t *d=(volatile uint8_t *)destination;
    for (size_t i=0;i<count;i++) d[i]=0;
}
static int valid_char(char c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||
           (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.';
}
static int normalize_path(const char *input,char output[VFS_PATH_MAX+1u]) {
    if (!input || !output) return -1;
    size_t length=str_len(input),start=0,out=0,component=0;
    if (length>VFS_PATH_MAX+1u) return -1;
    if (input[0]=='/') start=1;
    if (length>start && input[length-1u]=='/') --length;
    if (start==length) { output[0]=0; return 0; }
    for (size_t i=start;i<length;i++) {
        char c=input[i];
        if (c=='/') {
            if (!component || (component==1u && output[out-1u]=='.') ||
                (component==2u && output[out-1u]=='.' && output[out-2u]=='.'))
                return -1;
            if (out>=VFS_PATH_MAX) return -1;
            output[out++]='/';
            component=0;
        } else {
            if (!valid_char(c) || out>=VFS_PATH_MAX) return -1;
            output[out++]=c;
            ++component;
        }
    }
    if (!component || (component==1u && output[out-1u]=='.') ||
        (component==2u && output[out-1u]=='.' && output[out-2u]=='.'))
        return -1;
    output[out]=0;
    return 0;
}
static int mount_prefix(const char *path,const char *mount_path) {
    size_t n=str_len(mount_path);
    if (!n) return 1;
    if (!str_starts(path,mount_path)) return 0;
    return path[n]==0 || path[n]=='/';
}
static int resolve_path(const char *input,enum vfs_backend *backend,
                        const char **relative,char normalized[VFS_PATH_MAX+1u]) {
    if (normalize_path(input,normalized)<0) return -1;
    int best=-1;
    size_t best_length=0;
    for (uint32_t i=0;i<VFS_MOUNT_MAX;i++) {
        if (!mounts[i].used || !mount_prefix(normalized,mounts[i].path)) continue;
        size_t n=str_len(mounts[i].path);
        if (best<0 || n>best_length) { best=(int)i; best_length=n; }
    }
    if (best<0) return -1;
    *backend=mounts[best].backend;
    const char *rest=normalized+best_length;
    if (*rest=='/') ++rest;
    *relative=rest;
    return 0;
}

int vfs_mount(const char *mount_path,enum vfs_backend backend) {
    char normalized[VFS_PATH_MAX+1u];
    if (normalize_path(mount_path,normalized)<0 ||
        backend<VFS_BACKEND_RAM || backend>VFS_BACKEND_PROC)
        return -1;
    for (uint32_t i=0;i<VFS_MOUNT_MAX;i++) {
        if (mounts[i].used && str_eq(mounts[i].path,normalized)) return -1;
    }
    for (uint32_t i=0;i<VFS_MOUNT_MAX;i++) {
        if (mounts[i].used) continue;
        mounts[i].backend=backend;
        bytes_copy(mounts[i].path,normalized,str_len(normalized)+1u);
        mounts[i].used=1;
        return 0;
    }
    return -1;
}

void vfs_init(void) {
    for (uint32_t i=0;i<VFS_MOUNT_MAX;i++) mounts[i].used=0;
    for (uint32_t i=0;i<VFS_OPEN_MAX;i++) handles[i].used=0;
    (void)vfs_mount("/",pfs_ready()?VFS_BACKEND_BREADFS:VFS_BACKEND_RAM);
    (void)vfs_mount("/dev",VFS_BACKEND_DEV);
    (void)vfs_mount("/proc",VFS_BACKEND_PROC);
}

static size_t backend_count(enum vfs_backend backend) {
    if (backend==VFS_BACKEND_BREADFS) return pfs_count();
    if (backend==VFS_BACKEND_RAM) return fs_count();
    return 0;
}
static const char *backend_name(enum vfs_backend backend,size_t index) {
    if (backend==VFS_BACKEND_BREADFS) return pfs_name(index);
    if (backend==VFS_BACKEND_RAM) return fs_name(index);
    return 0;
}
static int backend_read_text(enum vfs_backend backend,const char *path,
                             char *out,size_t capacity) {
    if (backend==VFS_BACKEND_BREADFS) return pfs_read(path,out,capacity);
    if (backend==VFS_BACKEND_RAM) return fs_read(path,out,capacity);
    return -1;
}
static int backend_write_text(enum vfs_backend backend,const char *path,const char *data) {
    if (backend==VFS_BACKEND_BREADFS) return pfs_write(path,data);
    if (backend==VFS_BACKEND_RAM) return fs_write(path,data);
    return -1;
}
static int backend_exists(enum vfs_backend backend,const char *path,int *is_directory) {
    size_t n=str_len(path);
    if (!n || n>FS_NAME_MAX) return 0;
    for (size_t i=0;i<backend_count(backend);i++) {
        const char *name=backend_name(backend,i);
        if (!name) continue;
        if (str_eq(name,path)) { if(is_directory)*is_directory=0; return 1; }
        if (str_len(name)==n+1u && str_starts(name,path) && name[n]=='/') {
            if(is_directory)*is_directory=1;
            return 1;
        }
    }
    return 0;
}
static int backend_parent_exists(enum vfs_backend backend,const char *path) {
    size_t n=str_len(path);
    for(size_t i=0;i<n;i++) {
        if(path[i]!='/') continue;
        char parent[FS_NAME_MAX+1u];
        if(i>=FS_NAME_MAX)return 0;
        bytes_copy(parent,path,i);parent[i]=0;
        int is_directory=0;
        if(!backend_exists(backend,parent,&is_directory) || !is_directory)return 0;
    }
    return 1;
}
static int backend_read_binary(enum vfs_backend backend,const char *path,
                               void *out,size_t capacity) {
    if (backend!=VFS_BACKEND_BREADFS) return -1;
    return pfs_read_binary(path,out,capacity);
}
static int backend_write_binary(enum vfs_backend backend,const char *path,
                                const void *data,size_t size) {
    if (backend!=VFS_BACKEND_BREADFS) return -1;
    return pfs_write_binary(path,data,size);
}

static void append_char(char *out,size_t capacity,size_t *length,char c) {
    if (*length+1u<capacity) out[(*length)++]=c;
}
static void append_text(char *out,size_t capacity,size_t *length,const char *s) {
    while (*s) append_char(out,capacity,length,*s++);
}
static void append_u32(char *out,size_t capacity,size_t *length,uint32_t value) {
    char digits[10]; uint32_t count=0;
    do { digits[count++]=(char)('0'+value%10u); value/=10u; } while(value && count<10u);
    while(count) append_char(out,capacity,length,digits[--count]);
}
static void finish_text(char *out,size_t capacity,size_t length) {
    if (capacity) out[length<capacity?length:capacity-1u]=0;
}
static int build_pseudo_file(struct vfs_handle *h,const char *path) {
    size_t n=0;
    if (str_eq(path,"version")) {
        append_text((char *)h->data,sizeof(h->data),&n,"BreadOS native kernel, i386\n");
    } else if (str_eq(path,"uptime")) {
        uint32_t ticks=timer_ticks();
        append_u32((char *)h->data,sizeof(h->data),&n,ticks/100u);
        append_char((char *)h->data,sizeof(h->data),&n,'.');
        append_u32((char *)h->data,sizeof(h->data),&n,(ticks%100u)/10u);
        append_u32((char *)h->data,sizeof(h->data),&n,ticks%10u);
        append_text((char *)h->data,sizeof(h->data),&n," seconds\n");
    } else if (str_eq(path,"meminfo")) {
        append_text((char *)h->data,sizeof(h->data),&n,"free_frames ");
        append_u32((char *)h->data,sizeof(h->data),&n,memory_free_frames());
        append_text((char *)h->data,sizeof(h->data),&n,"\n");
    } else if (str_eq(path,"tasks")) {
        uint32_t ids[TASK_MAX]; uint8_t states[TASK_MAX];
        uint32_t count=process_snapshot(ids,states,TASK_MAX);
        for(uint32_t i=0;i<count;i++) {
            append_text((char *)h->data,sizeof(h->data),&n,"task ");
            append_u32((char *)h->data,sizeof(h->data),&n,ids[i]);
            append_text((char *)h->data,sizeof(h->data),&n,states[i]?" exiting\n":" running\n");
        }
    } else return -1;
    finish_text((char *)h->data,sizeof(h->data),n);
    h->size=(uint32_t)n;
    return 0;
}
static int device_id(const char *path) {
    if (str_eq(path,"null")) return 1;
    if (str_eq(path,"zero")) return 2;
    if (str_eq(path,"console")) return 3;
    if (str_eq(path,"serial")) return 4;
    if (str_eq(path,"uptime")) return 5;
    if (str_eq(path,"meminfo")) return 6;
    return 0;
}
static int proc_name(const char *path) {
    return str_eq(path,"version") || str_eq(path,"uptime") ||
           str_eq(path,"meminfo") || str_eq(path,"tasks");
}
static struct vfs_handle *handle_at(int handle) {
    if (handle<0 || (uint32_t)handle>=VFS_OPEN_MAX || !handles[handle].used) return 0;
    return &handles[handle];
}

int vfs_open(const char *path,uint32_t flags) {
    if (!(flags&(VFS_O_READ|VFS_O_WRITE)) ||
        ((flags&VFS_O_CREATE) && !(flags&VFS_O_WRITE)) ||
        (flags&~(VFS_O_READ|VFS_O_WRITE|VFS_O_CREATE|VFS_O_TRUNCATE|VFS_O_BINARY)) ||
        ((flags&VFS_O_TRUNCATE) && !(flags&VFS_O_WRITE)))
        return -1;
    enum vfs_backend backend; const char *relative;
    char normalized[VFS_PATH_MAX+1u];
    if (resolve_path(path,&backend,&relative,normalized)<0 || !*relative) return -1;
    int slot=-1;
    for(uint32_t i=0;i<VFS_OPEN_MAX;i++) if(!handles[i].used){slot=(int)i;break;}
    if(slot<0) return -1;
    struct vfs_handle *h=&handles[slot];
    h->backend=backend;h->flags=flags;h->position=0;h->size=0;h->dirty=0;h->device=0;
    bytes_copy(h->path,relative,str_len(relative)+1u);
    if(backend==VFS_BACKEND_DEV) {
        h->device=(uint8_t)device_id(relative);
        if(!h->device) return -1;
        if((h->device==5u || h->device==6u) && (flags&VFS_O_READ))
            (void)build_pseudo_file(h,h->device==5u?"uptime":"meminfo");
        if((h->device==5u || h->device==6u) && (flags&VFS_O_WRITE)) return -1;
        if((flags&VFS_O_BINARY) || (flags&VFS_O_TRUNCATE)) return -1;
        h->used=1;
        return slot;
    }
    if(backend==VFS_BACKEND_PROC) {
        if(!(flags&VFS_O_READ) || (flags&(VFS_O_WRITE|VFS_O_CREATE|VFS_O_TRUNCATE|VFS_O_BINARY)) ||
           !proc_name(relative) || build_pseudo_file(h,relative)<0) return -1;
        h->used=1;
        return slot;
    }
    if(str_len(relative)>FS_NAME_MAX)return -1;
    int is_directory=0;
    int exists=backend_exists(backend,relative,&is_directory);
    if(!exists && (flags&VFS_O_CREATE) && !backend_parent_exists(backend,relative))return -1;
    if(is_directory) return -1;
    if(flags&VFS_O_BINARY) {
        if(backend!=VFS_BACKEND_BREADFS) return -1;
        if(exists) {
            int size=backend_read_binary(backend,relative,h->data,sizeof(h->data));
            if(size<0) return -1;
            h->size=(uint32_t)size;
        } else if(!(flags&VFS_O_CREATE)) return -1;
    } else if(exists) {
        char text[FS_DATA_MAX+1u];
        int size=backend_read_text(backend,relative,text,sizeof(text));
        if(size<0) return -1;
        h->size=(uint32_t)size;
        bytes_copy(h->data,text,(size_t)size);
    } else if(!(flags&VFS_O_CREATE)) return -1;
    if(!exists && (flags&VFS_O_CREATE)) h->dirty=1;
    if(flags&VFS_O_TRUNCATE) { h->size=0; h->dirty=1; }
    h->used=1;
    return slot;
}

static void discard_handle(struct vfs_handle *h) {
    if(!h)return;
    h->used=0;
    h->dirty=0;
    bytes_zero(h->data,sizeof(h->data));
}
int vfs_close(int handle) {
    struct vfs_handle *h=handle_at(handle);
    if(!h)return -1;
    int result=0;
    if(h->dirty) {
        if(h->flags&VFS_O_BINARY)
            result=backend_write_binary(h->backend,h->path,h->data,h->size);
        else {
            if(h->size>FS_DATA_MAX) result=-1;
            else {
                h->data[h->size]=0;
                result=backend_write_text(h->backend,h->path,(const char *)h->data);
            }
        }
    }
    discard_handle(h);
    return result<0?-1:0;
}

