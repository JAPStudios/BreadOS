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
    uint8_t created;
    uint32_t flags;
    uint32_t owner;
    enum vfs_backend backend;
    uint32_t position;
    uint32_t size;
    char path[VFS_PATH_MAX+1u];
    uint8_t data[VFS_FILE_MAX];
};

static struct vfs_mount_entry mounts[VFS_MOUNT_MAX];
static struct vfs_handle handles[TASK_MAX][VFS_OPEN_MAX];

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


#define VFS_ACL_MAX 16u
#define VFS_ACL_MAGIC 0x314C4341u
#define VFS_ACL_VERSION 1u
#define ACL_READ 4u
#define ACL_WRITE 2u
#define ACL_EXEC 1u
#define ACL_ROOT_MODE 0777u
struct vfs_acl_record {
    char path[VFS_PATH_MAX+1u];
    uint32_t owner;
    uint16_t mode;
    uint8_t type;
    uint8_t used;
} __attribute__((packed));
struct vfs_acl_disk {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    uint32_t checksum;
    struct vfs_acl_record records[VFS_ACL_MAX];
} __attribute__((packed));
static struct vfs_acl_record acl_records[VFS_ACL_MAX];

static uint32_t acl_current_owner(void) {
    return task_current_id()==0u?0u:1u;
}
static int acl_reserved_path(const char *path) {
    return str_eq(path,".vfsmeta");
}
static int acl_find(const char *path) {
    for(uint32_t i=0;i<VFS_ACL_MAX;i++)
        if(acl_records[i].used && str_eq(acl_records[i].path,path)) return (int)i;
    return -1;
}
static void acl_get(const char *path,enum vfs_node_type type,
                    uint32_t *owner,uint16_t *mode) {
    if(!path || !*path) {
        if(owner)*owner=0;
        if(mode)*mode=ACL_ROOT_MODE;
        return;
    }
    int index=acl_find(path);
    if(index>=0) {
        if(owner)*owner=acl_records[index].owner;
        if(mode)*mode=acl_records[index].mode;
        return;
    }
    if(owner)*owner=0;
    if(mode)*mode=(uint16_t)(type==VFS_NODE_DIRECTORY?0755u:0644u);
}
static int acl_access(const char *path,enum vfs_node_type type,uint8_t required) {
    uint32_t principal=acl_current_owner(),owner=0;
    uint16_t mode=0;
    if(principal==0) return 1;
    acl_get(path,type,&owner,&mode);
    uint8_t bits=(uint8_t)(principal==owner?((mode>>6)&7u):(mode&7u));
    return (bits&required)==required;
}
static int acl_parent_access(const char *path,int need_write) {
    uint8_t required=(uint8_t)(ACL_EXEC|(need_write?ACL_WRITE:0u));
    if(!acl_access("",VFS_NODE_DIRECTORY,required)) return 0;
    size_t n=str_len(path);
    char parent[VFS_PATH_MAX+1u];
    for(size_t i=0;i<n;i++) {
        if(path[i]!='/') continue;
        if(!i || i>VFS_PATH_MAX) return 0;
        bytes_copy(parent,path,i);
        parent[i]=0;
        if(!acl_access(parent,VFS_NODE_DIRECTORY,required)) return 0;
    }
    return 1;
}
static int acl_owner_or_root(const char *path) {
    uint32_t principal=acl_current_owner(),owner=0;
    if(principal==0) return 1;
    acl_get(path,VFS_NODE_FILE,&owner,0);
    return owner==principal;
}
static int acl_set(const char *path,uint32_t owner,uint16_t mode,
                   enum vfs_node_type type) {
    if(!path || !*path || str_len(path)>VFS_PATH_MAX || owner>1u ||
       (type!=VFS_NODE_FILE && type!=VFS_NODE_DIRECTORY)) return -1;
    int slot=acl_find(path);
    if(slot<0) {
        for(uint32_t i=0;i<VFS_ACL_MAX;i++)
            if(!acl_records[i].used) {slot=(int)i;break;}
    }
    if(slot<0) return -1;
    struct vfs_acl_record *r=&acl_records[slot];
    bytes_zero(r,sizeof(*r));
    bytes_copy(r->path,path,str_len(path)+1u);
    r->owner=owner;r->mode=(uint16_t)(mode&0777u);
    r->type=(uint8_t)type;r->used=1;
    return 0;
}
static void acl_remove(const char *path) {
    int slot=acl_find(path);
    if(slot>=0) bytes_zero(&acl_records[slot],sizeof(acl_records[slot]));
}
static int acl_rename(const char *source,const char *destination) {
    int slot=acl_find(source);
    if(slot<0) return 0;
    if(acl_find(destination)>=0 || str_len(destination)>VFS_PATH_MAX) return -1;
    bytes_zero(acl_records[slot].path,sizeof(acl_records[slot].path));
    bytes_copy(acl_records[slot].path,destination,str_len(destination)+1u);
    return 0;
}
static uint32_t acl_checksum(const struct vfs_acl_disk *disk) {
    uint32_t hash=2166136261u;
    const uint8_t *bytes=(const uint8_t *)disk->records;
    for(size_t i=0;i<sizeof(disk->records);i++)
        hash=(hash^bytes[i])*16777619u;
    hash=(hash^disk->magic)*16777619u;
    hash=(hash^disk->version)*16777619u;
    hash=(hash^disk->count)*16777619u;
    return hash;
}
static int acl_save(void) {
    if(!pfs_ready()) return 0;
    struct vfs_acl_disk disk;
    bytes_zero(&disk,sizeof(disk));
    disk.magic=VFS_ACL_MAGIC;disk.version=VFS_ACL_VERSION;
    for(uint32_t i=0;i<VFS_ACL_MAX;i++) {
        disk.records[i]=acl_records[i];
        if(acl_records[i].used) ++disk.count;
    }
    disk.checksum=acl_checksum(&disk);
    int result=pfs_write_binary(".vfsmeta",&disk,sizeof(disk));
    return result==(int)sizeof(disk)?0:-1;
}
static void acl_init(void) {
    bytes_zero(acl_records,sizeof(acl_records));
    if(!pfs_ready()) return;
    struct vfs_acl_disk disk;
    bytes_zero(&disk,sizeof(disk));
    int size=pfs_read_binary(".vfsmeta",&disk,sizeof(disk));
    if(size!=(int)sizeof(disk) || disk.magic!=VFS_ACL_MAGIC ||
       disk.version!=VFS_ACL_VERSION || disk.count>VFS_ACL_MAX ||
       disk.checksum!=acl_checksum(&disk)) return;
    uint32_t valid=0;
    for(uint32_t i=0;i<VFS_ACL_MAX;i++) {
        struct vfs_acl_record *r=&disk.records[i];
        if(!r->used || r->owner>1u ||
           (r->type!=VFS_NODE_FILE && r->type!=VFS_NODE_DIRECTORY)) continue;
        char normalized[VFS_PATH_MAX+1u];
        if(normalize_path(r->path,normalized)<0 || !str_eq(normalized,r->path) ||
           acl_reserved_path(r->path)) continue;
        acl_records[i]=*r;
        ++valid;
    }
    (void)valid;
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
    for (uint32_t task=0;task<TASK_MAX;task++)
        for (uint32_t i=0;i<VFS_OPEN_MAX;i++) handles[task][i].used=0;
    (void)vfs_mount("/",pfs_ready()?VFS_BACKEND_BREADFS:VFS_BACKEND_RAM);
    (void)vfs_mount("/dev",VFS_BACKEND_DEV);
    (void)vfs_mount("/proc",VFS_BACKEND_PROC);
    acl_init();
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
    uint32_t owner=task_current_id();
    if (owner>=TASK_MAX || handle<0 || (uint32_t)handle>=VFS_OPEN_MAX ||
        !handles[owner][handle].used) return 0;
    return &handles[owner][handle];
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
    uint32_t owner=task_current_id();
    if(owner>=TASK_MAX) return -1;
    int slot=-1;
    for(uint32_t i=0;i<VFS_OPEN_MAX;i++)
        if(!handles[owner][i].used){slot=(int)i;break;}
    if(slot<0) return -1;
    struct vfs_handle *h=&handles[owner][slot];
    h->backend=backend;h->flags=flags;h->owner=owner;
    h->position=0;h->size=0;h->dirty=0;h->device=0;h->created=0;
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
    if(acl_reserved_path(relative)) return -1;
    int is_directory=0;
    int exists=backend_exists(backend,relative,&is_directory);
    if(is_directory) return -1;
    if(exists) {
        uint8_t required=(uint8_t)(((flags&VFS_O_READ)?ACL_READ:0u)|
                                   ((flags&VFS_O_WRITE)?ACL_WRITE:0u));
        if(!acl_parent_access(normalized,0) ||
           !acl_access(normalized,VFS_NODE_FILE,required)) return -1;
    } else if(flags&VFS_O_CREATE) {
        if(!backend_parent_exists(backend,relative) ||
           !acl_parent_access(normalized,1)) return -1;
    } else return -1;
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
    if(!exists && (flags&VFS_O_CREATE)) { h->dirty=1;h->created=1; }
    if(flags&VFS_O_TRUNCATE) { h->size=0; h->dirty=1; }
    h->used=1;
    return slot;
}

static void discard_handle(struct vfs_handle *h) {
    if(!h)return;
    h->used=0;
    h->dirty=0;
    h->created=0;
    bytes_zero(h->data,sizeof(h->data));
}
static int finish_handle(struct vfs_handle *h) {
    if(!h || !h->used) return -1;
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
        if(result>=0 && h->created) {
            uint32_t principal=h->owner==0u?0u:1u;
            uint16_t mode=(uint16_t)(principal==0u?0644u:0600u);
            if(acl_set(h->path,principal,mode,VFS_NODE_FILE)<0 || acl_save()<0)
                result=-1;
        }
    }
    discard_handle(h);
    return result<0?-1:0;
}

int vfs_close(int handle) {
    return finish_handle(handle_at(handle));
}

void vfs_close_task(uint32_t task_id) {
    if(task_id>=TASK_MAX) return;
    for(uint32_t i=0;i<VFS_OPEN_MAX;i++) {
        if(handles[task_id][i].used)
            (void)finish_handle(&handles[task_id][i]);
    }
}

int vfs_read(int handle,void *buffer,size_t count) {
    struct vfs_handle *h=handle_at(handle);
    if(!h || !(h->flags&VFS_O_READ) || (!buffer&&count)) return -1;
    if(!count) return 0;
    if(h->backend==VFS_BACKEND_DEV) {
        uint8_t *out=(uint8_t *)buffer;
        if(h->device==1u) return 0;
        if(h->device==2u) { for(size_t i=0;i<count;i++)out[i]=0;return (int)count; }
        if(h->device==3u) {
            size_t n=0;char ch;
            while(n<count && keyboard_read_char(&ch))out[n++]=(uint8_t)ch;
            return n?(int)n:-1;
        }
        if(h->device==4u) return -1;
    }
    if(h->position>=h->size) return 0;
    size_t remaining=h->size-h->position;
    if(count>remaining) count=remaining;
    bytes_copy(buffer,h->data+h->position,count);
    h->position+=(uint32_t)count;
    return (int)count;
}

int vfs_write(int handle,const void *buffer,size_t count) {
    struct vfs_handle *h=handle_at(handle);
    if(!h || !(h->flags&VFS_O_WRITE) || (!buffer&&count)) return -1;
    if(!count) return 0;
    if(h->backend==VFS_BACKEND_DEV) {
        if(h->device==1u || h->device==2u) return (int)count;
        if(h->device==3u || h->device==4u) {
            const char *s=(const char *)buffer;
            for(size_t i=0;i<count;i++) console_put_char(s[i]);
            return (int)count;
        }
        return -1;
    }
    uint32_t limit=(h->flags&VFS_O_BINARY)?VFS_FILE_MAX:FS_DATA_MAX;
    if(h->position>limit || count>limit-h->position) return -1;
    const uint8_t *source=(const uint8_t *)buffer;
    if(!(h->flags&VFS_O_BINARY))
        for(size_t i=0;i<count;i++) if(source[i]==0) return -1;
    if(h->position>h->size) bytes_zero(h->data+h->size,h->position-h->size);
    bytes_copy(h->data+h->position,buffer,count);
    h->position+=(uint32_t)count;
    if(h->position>h->size) h->size=h->position;
    h->dirty=1;
    return (int)count;
}

int vfs_seek(int handle,int32_t offset,int whence) {
    struct vfs_handle *h=handle_at(handle);
    if(!h || h->backend==VFS_BACKEND_DEV || h->backend==VFS_BACKEND_PROC) return -1;
    int64_t base;
    if(whence==0) base=0;
    else if(whence==1) base=(int64_t)h->position;
    else if(whence==2) base=(int64_t)h->size;
    else return -1;
    int64_t next=base+offset;
    if(next<0 || next>VFS_FILE_MAX) return -1;
    h->position=(uint32_t)next;
    return 0;
}

int vfs_stat_path(const char *path,struct vfs_stat *status) {
    if(!status) return -1;
    enum vfs_backend backend;const char *relative;char normalized[VFS_PATH_MAX+1u];
    if(resolve_path(path,&backend,&relative,normalized)<0) return -1;
    if(backend!=VFS_BACKEND_DEV && backend!=VFS_BACKEND_PROC &&
       acl_reserved_path(relative)) return -1;
    if(!*relative) {
        if(backend!=VFS_BACKEND_DEV && backend!=VFS_BACKEND_PROC &&
           !acl_access("",VFS_NODE_DIRECTORY,ACL_READ|ACL_EXEC)) return -1;
        status->type=VFS_NODE_DIRECTORY;status->size=0;return 0;
    }
    if(backend==VFS_BACKEND_DEV) {
        if(!device_id(relative))return -1;
        status->type=VFS_NODE_DEVICE;status->size=0;return 0;
    }
    if(backend==VFS_BACKEND_PROC) {
        if(!proc_name(relative))return -1;
        status->type=VFS_NODE_FILE;status->size=0;return 0;
    }
    if(!acl_parent_access(normalized,0)) return -1;
    int is_directory=0;
    if(!backend_exists(backend,relative,&is_directory))return -1;
    enum vfs_node_type type=is_directory?VFS_NODE_DIRECTORY:VFS_NODE_FILE;
    uint8_t required=is_directory?(ACL_READ|ACL_EXEC):ACL_READ;
    if(!acl_access(normalized,type,required)) return -1;
    status->type=type;
    status->size=0;
    return 0;
}

static int add_dirent(struct vfs_dirent *entries,size_t *count,
                      const char *name,enum vfs_node_type type) {
    size_t n=str_len(name);
    if(!n || n>15u) return 0;
    for(size_t i=0;i<*count;i++) if(str_eq(entries[i].name,name)) return 0;
    struct vfs_dirent *e=&entries[(*count)++];
    bytes_copy(e->name,name,n);e->name[n]=0;e->type=type;e->size=0;
    return 1;
}

int vfs_readdir(const char *path,size_t index,struct vfs_dirent *entry) {
    if(!entry) return -1;
    struct vfs_stat status;
    if(vfs_stat_path(path,&status)<0 || status.type!=VFS_NODE_DIRECTORY) return -1;
    enum vfs_backend backend;const char *relative;char normalized[VFS_PATH_MAX+1u];
    if(resolve_path(path,&backend,&relative,normalized)<0) return -1;
    struct vfs_dirent entries[FS_MAX_FILES+2u];size_t count=0;
    if(backend==VFS_BACKEND_DEV) {
        static const char *names[]={"null","zero","console","serial","uptime","meminfo"};
        for(size_t i=0;i<sizeof(names)/sizeof(names[0]);i++)
            add_dirent(entries,&count,names[i],VFS_NODE_DEVICE);
    } else if(backend==VFS_BACKEND_PROC) {
        static const char *names[]={"version","uptime","meminfo","tasks"};
        for(size_t i=0;i<sizeof(names)/sizeof(names[0]);i++)
            add_dirent(entries,&count,names[i],VFS_NODE_FILE);
    } else {
        if(!*relative) {
            add_dirent(entries,&count,"dev",VFS_NODE_DIRECTORY);
            add_dirent(entries,&count,"proc",VFS_NODE_DIRECTORY);
        }
        size_t prefix_length=str_len(relative);
        char prefix[VFS_PATH_MAX+1u];
        if(prefix_length) {
            bytes_copy(prefix,relative,prefix_length);
            prefix[prefix_length++]='/';
        }
        prefix[prefix_length]=0;
        for(size_t i=0;i<backend_count(backend);i++) {
            const char *stored=backend_name(backend,i);
            if(!stored || acl_reserved_path(stored) || !str_starts(stored,prefix)) continue;
            const char *rest=stored+prefix_length;
            if(!*rest) continue;
            size_t child_length=0;
            while(rest[child_length] && rest[child_length]!='/') ++child_length;
            if(!child_length || child_length>15u) continue;
            char child[16];
            bytes_copy(child,rest,child_length);child[child_length]=0;
            enum vfs_node_type type=rest[child_length]=='/'?
                VFS_NODE_DIRECTORY:VFS_NODE_FILE;
            char full[VFS_PATH_MAX+1u];
            size_t parent_length=str_len(normalized);
            if(parent_length) {
                bytes_copy(full,normalized,parent_length);
                full[parent_length++]='/';
            }
            if(parent_length+child_length>VFS_PATH_MAX) continue;
            bytes_copy(full+parent_length,child,child_length);
            full[parent_length+child_length]=0;
            uint8_t required=type==VFS_NODE_DIRECTORY?
                (ACL_READ|ACL_EXEC):ACL_READ;
            if(!acl_parent_access(full,0) || !acl_access(full,type,required)) continue;
            add_dirent(entries,&count,child,type);
        }
    }
    if(index>=count) return -1;
    *entry=entries[index];
    return 0;
}

static int resolve_backend_mutation(const char *path,enum vfs_backend *backend,
                                    const char **relative,
                                    char normalized[VFS_PATH_MAX+1u]) {
    if(resolve_path(path,backend,relative,normalized)<0 || !**relative ||
       (*backend!=VFS_BACKEND_RAM && *backend!=VFS_BACKEND_BREADFS) ||
       str_len(*relative)>FS_NAME_MAX) return -1;
    return 0;
}
int vfs_mkdir(const char *path) {
    enum vfs_backend b;const char *r;char n[VFS_PATH_MAX+1u];
    if(resolve_backend_mutation(path,&b,&r,n)<0 || acl_reserved_path(r) ||
       !acl_parent_access(n,1)) return -1;
    int result=b==VFS_BACKEND_BREADFS?pfs_mkdir(r):fs_mkdir(r);
    if(result<0) return result;
    uint32_t owner=acl_current_owner();
    uint16_t mode=(uint16_t)(owner==0u?0755u:0700u);
    if(acl_set(n,owner,mode,VFS_NODE_DIRECTORY)<0 || acl_save()<0) {
        if(b==VFS_BACKEND_BREADFS)(void)pfs_rmdir(r);else (void)fs_rmdir(r);
        acl_remove(n);
        return -1;
    }
    return 0;
}
int vfs_unlink(const char *path) {
    enum vfs_backend b;const char *r;char n[VFS_PATH_MAX+1u];
    if(resolve_backend_mutation(path,&b,&r,n)<0 || acl_reserved_path(r) ||
       !acl_parent_access(n,1) || !acl_owner_or_root(n)) return -1;
    int result=b==VFS_BACKEND_BREADFS?pfs_remove(r):fs_remove(r);
    if(result<0) return result;
    acl_remove(n);(void)acl_save();
    return 0;
}
int vfs_rmdir(const char *path) {
    enum vfs_backend b;const char *r;char n[VFS_PATH_MAX+1u];
    if(resolve_backend_mutation(path,&b,&r,n)<0 || acl_reserved_path(r) ||
       !acl_parent_access(n,1) || !acl_owner_or_root(n)) return -1;
    int result=b==VFS_BACKEND_BREADFS?pfs_rmdir(r):fs_rmdir(r);
    if(result<0) return result;
    acl_remove(n);(void)acl_save();
    return 0;
}
int vfs_rename(const char *source,const char *destination) {
    enum vfs_backend sb,db;const char *sr,*dr;
    char sn[VFS_PATH_MAX+1u],dn[VFS_PATH_MAX+1u];
    if(resolve_backend_mutation(source,&sb,&sr,sn)<0 ||
       resolve_backend_mutation(destination,&db,&dr,dn)<0 || sb!=db ||
       acl_reserved_path(sr) || acl_reserved_path(dr) ||
       !acl_owner_or_root(sn) || !acl_parent_access(sn,1) ||
       !acl_parent_access(dn,1)) return -1;
    int result=sb==VFS_BACKEND_BREADFS?pfs_move(sr,dr):fs_move(sr,dr);
    if(result<0) return result;
    if(acl_rename(sn,dn)<0) return -1;
    (void)acl_save();
    return 0;
}

int vfs_read_all(const char *path,void *buffer,size_t capacity,int binary) {
    if(!buffer || !capacity) return -1;
    int fd=vfs_open(path,VFS_O_READ|(binary?VFS_O_BINARY:0));
    if(fd<0) return -1;
    int total=0;
    while((size_t)total<capacity) {
        int n=vfs_read(fd,(uint8_t *)buffer+total,capacity-(size_t)total);
        if(n<0) { (void)vfs_close(fd);return -1; }
        if(!n) break;
        total+=n;
    }
    struct vfs_handle *h=handle_at(fd);
    int truncated=h && h->position<h->size;
    (void)vfs_close(fd);
    if(truncated) return -2;
    if(!binary && (size_t)total<capacity) ((char *)buffer)[total]=0;
    return total;
}
int vfs_write_all(const char *path,const void *buffer,size_t size,int binary) {
    if((!buffer&&size) || size>VFS_FILE_MAX) return -1;
    int fd=vfs_open(path,VFS_O_WRITE|VFS_O_CREATE|VFS_O_TRUNCATE|
                    (binary?VFS_O_BINARY:0));
    if(fd<0) return -1;
    int written=vfs_write(fd,buffer,size);
    if(written<0 || (size_t)written!=size) {
        discard_handle(handle_at(fd));
        return -1;
    }
    if(vfs_close(fd)<0) return -1;
    return written;
}
