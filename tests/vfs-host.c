#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "vfs.h"
#include "fs.h"
#include "pfs.h"
#include "memory.h"
#include "interrupts.h"
#include "process.h"
#include "console.h"
#include "kernel/keyboard.h"

static char console_output[64];
static size_t console_length;
static uint32_t active_task_id;
uint32_t task_current_id(void){return active_task_id;}
int pfs_init(void){return 0;}
int pfs_ready(void){return 0;}
size_t pfs_count(void){return 0;}
const char *pfs_name(size_t i){(void)i;return 0;}
int pfs_read(const char *p,char *o,size_t c){(void)p;(void)o;(void)c;return -1;}
int pfs_read_binary(const char *p,void *o,size_t c){(void)p;(void)o;(void)c;return -1;}
int pfs_write(const char *p,const char *d){(void)p;(void)d;return -1;}
int pfs_write_binary(const char *p,const void *d,size_t n){(void)p;(void)d;(void)n;return -1;}
int pfs_remove(const char *p){(void)p;return -1;}
int pfs_mkdir(const char *p){(void)p;return -1;}
int pfs_rmdir(const char *p){(void)p;return -1;}
int pfs_move(const char *a,const char *b){(void)a;(void)b;return -1;}
uint32_t memory_free_frames(void){return 123u;}
uint32_t timer_ticks(void){return 12345u;}
uint32_t process_snapshot(uint32_t *ids,uint8_t *states,uint32_t capacity){
    if(capacity){ids[0]=3;states[0]=0;return 1;}return 0;
}
void console_put_char(char c){if(console_length<sizeof(console_output))console_output[console_length++]=c;}
int keyboard_read_char(char *out){(void)out;return 0;}

int main(void) {
    char buffer[128];
    struct vfs_stat status;
    struct vfs_dirent entry;
    fs_init();
    vfs_init();

    assert(vfs_stat_path("/", &status)==0 && status.type==VFS_NODE_DIRECTORY);
    assert(vfs_stat_path("/dev/null", &status)==0 && status.type==VFS_NODE_DEVICE);
    assert(vfs_stat_path("/proc/tasks", &status)==0 && status.type==VFS_NODE_FILE);
    assert(vfs_open("/../escape",VFS_O_READ)<0);
    assert(vfs_open("/dev",VFS_O_READ)<0);
    assert(vfs_mount("/dev",VFS_BACKEND_DEV)<0);
    assert(vfs_open("/missing/a.txt",VFS_O_WRITE|VFS_O_CREATE)<0);

    assert(vfs_mkdir("/docs")==0);
    assert(vfs_write_all("/docs/a.txt","alpha",5,0)==5);
    assert(vfs_read_all("/docs/a.txt",buffer,sizeof(buffer),0)==5);
    assert(strcmp(buffer,"alpha")==0);
    assert(vfs_readdir("/docs/",0,&entry)==0);
    assert(strcmp(entry.name,"a.txt")==0 && entry.type==VFS_NODE_FILE);
    assert(vfs_rename("/docs/a.txt","/docs/b.txt")==0);
    assert(vfs_read_all("/docs/b.txt",buffer,sizeof(buffer),0)==5);
    assert(strcmp(buffer,"alpha")==0);
    assert(vfs_unlink("/docs/b.txt")==0);
    assert(vfs_rmdir("/docs")==0);

    assert(vfs_write_all("/empty.txt",0,0,0)==0);
    assert(vfs_read_all("/empty.txt",buffer,sizeof(buffer),0)==0);
    assert(buffer[0]==0);

    int fd=vfs_open("/empty.txt",VFS_O_READ|VFS_O_WRITE);
    assert(fd>=0);
    assert(vfs_write(fd,"xyz",3)==3);
    assert(vfs_seek(fd,0,0)==0);
    assert(vfs_read(fd,buffer,3)==3);
    assert(memcmp(buffer,"xyz",3)==0);
    assert(vfs_seek(fd,100,0)==0);
    assert(vfs_read(fd,buffer,1)==0);
    assert(vfs_close(fd)==0);

    /* The fixed VFS handle table must reject overflow and recover on close. */
    int handles[VFS_OPEN_MAX];
    for(unsigned i=0;i<VFS_OPEN_MAX;i++) {
        handles[i]=vfs_open("/empty.txt",VFS_O_READ);
        assert(handles[i]>=0);
    }
    assert(vfs_open("/empty.txt",VFS_O_READ)<0);
    assert(vfs_close(handles[1])==0);
    handles[1]=vfs_open("/empty.txt",VFS_O_READ);
    assert(handles[1]>=0);
    for(unsigned i=0;i<VFS_OPEN_MAX;i++) assert(vfs_close(handles[i])==0);

    /* Each task has an independent descriptor table, including identical fd numbers. */
    active_task_id=3;
    int task3_fd=vfs_open("/empty.txt",VFS_O_READ);
    assert(task3_fd==0);
    active_task_id=4;
    int task4_fd=vfs_open("/empty.txt",VFS_O_READ);
    assert(task4_fd==0);
    assert(vfs_read(task4_fd,buffer,1)==0);
    active_task_id=3;
    assert(vfs_read(task3_fd,buffer,1)==0);
    assert(vfs_close(task3_fd)==0);
    active_task_id=4;
    assert(vfs_close(task4_fd)==0);

    /* A task's descriptor capacity is independent of other tasks. */
    active_task_id=3;
    for(unsigned i=0;i<VFS_OPEN_MAX;i++)
        handles[i]=vfs_open("/empty.txt",VFS_O_READ);
    for(unsigned i=0;i<VFS_OPEN_MAX;i++) assert(handles[i]>=0);
    active_task_id=4;
    for(unsigned i=0;i<VFS_OPEN_MAX;i++)
        assert(vfs_open("/empty.txt",VFS_O_READ)>=0);
    active_task_id=3;
    vfs_close_task(3);
    active_task_id=4;
    vfs_close_task(4);
    active_task_id=0;

    /* Process exit closes owned handles and commits completed file writes. */
    active_task_id=3;
    fd=vfs_open("/exit.txt",VFS_O_WRITE|VFS_O_CREATE|VFS_O_TRUNCATE);
    assert(fd>=0 && vfs_write(fd,"saved",5)==5);
    active_task_id=4;
    vfs_close_task(3);
    active_task_id=0;
    assert(vfs_read_all("/exit.txt",buffer,sizeof(buffer),0)==5);
    assert(strcmp(buffer,"saved")==0);

    /* Invalid device opens must not consume a descriptor. */
    assert(vfs_open("/dev/not-a-device",VFS_O_READ)<0);
    fd=vfs_open("/empty.txt",VFS_O_READ);
    assert(fd>=0 && vfs_close(fd)==0);

    fd=vfs_open("/dev/null",VFS_O_WRITE);
    assert(fd>=0 && vfs_write(fd,"discard",7)==7 && vfs_close(fd)==0);
    fd=vfs_open("/dev/zero",VFS_O_READ);
    assert(fd>=0 && vfs_read(fd,buffer,8)==8);
    for(unsigned i=0;i<8;i++)assert(buffer[i]==0);
    assert(vfs_close(fd)==0);

    assert(vfs_read_all("/proc/version",buffer,sizeof(buffer),0)>0);
    assert(strstr(buffer,"BreadOS native kernel")!=0);
    assert(vfs_read_all("/proc/meminfo",buffer,sizeof(buffer),0)>0);
    assert(strstr(buffer,"123")!=0);
    assert(vfs_read_all("/proc/tasks",buffer,sizeof(buffer),0)>0);
    assert(strstr(buffer,"task 3 running")!=0);

    fd=vfs_open("/dev/console",VFS_O_WRITE);
    assert(fd>=0 && vfs_write(fd,"OK",2)==2 && vfs_close(fd)==0);
    assert(console_length==2 && console_output[0]=='O' && console_output[1]=='K');

    assert(vfs_readdir("/",0,&entry)==0);
    assert(strcmp(entry.name,"dev")==0 && entry.type==VFS_NODE_DIRECTORY);
    return 0;
}
