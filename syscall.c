#include "syscall.h"
#include "task.h"
#include "paging.h"
#include "ipc.h"
#include "console.h"
#include "process.h"
#include "vfs.h"
#include <stdint.h>

#define SYSCALL_ERROR 0xFFFFFFFFu

struct syscall_frame {
    uint32_t edi, esi, ebp, ignored_esp, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};

static int copy_from_user(void *destination,uint32_t source,uint32_t length) {
    const struct paging_address_space *space=task_current_address_space();
    if (!destination || !paging_user_range_valid(space,source,length,0))
        return -1;
    const volatile uint8_t *input=(const volatile uint8_t *)(uintptr_t)source;
    uint8_t *output=(uint8_t *)destination;
    for (uint32_t i=0;i<length;i++) output[i]=input[i];
    return 0;
}

static int copy_to_user(uint32_t destination,const void *source,uint32_t length) {
    const struct paging_address_space *space=task_current_address_space();
    if (!source || !paging_user_range_valid(space,destination,length,1))
        return -1;
    volatile uint8_t *output=(volatile uint8_t *)(uintptr_t)destination;
    const uint8_t *input=(const uint8_t *)source;
    for (uint32_t i=0;i<length;i++) output[i]=input[i];
    return 0;
}


static int copy_user_string(char *destination,uint32_t source,uint32_t capacity) {
    const struct paging_address_space *space=task_current_address_space();
    if(!destination || !source || !capacity)return -1;
    for(uint32_t i=0;i<capacity;i++) {
        if(source>0xFFFFFFFFu-i ||
           !paging_user_range_valid(space,source+i,1u,0))return -1;
        char value=(char)*(const volatile uint8_t *)(uintptr_t)(source+i);
        destination[i]=value;
        if(!value)return 0;
    }
    return -1;
}

int syscall_dispatch(uint32_t *saved_frame) {
    if (!saved_frame) return 0;
    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs&3u)!=3u) {
        frame->eax=SYSCALL_ERROR;
        return 0;
    }
    if (frame->eax==1u) {
        console_put_char((char)(frame->ebx&0xFFu));
        frame->eax=0;
        return 0;
    }
    if (frame->eax==2u)
        return task_terminate_current((int)frame->ebx)==0;

    if (frame->eax==3u) {
        uint8_t message[IPC_MESSAGE_MAX];
        uint32_t length=frame->edx;
        if (length>IPC_MESSAGE_MAX ||
            copy_from_user(message,frame->ecx,length)<0 ||
            ipc_send(task_current_id(),frame->ebx,message,length)<0)
            frame->eax=SYSCALL_ERROR;
        else
            frame->eax=0;
        return 0;
    }

    if (frame->eax==4u) {
        uint8_t message[IPC_MESSAGE_MAX];
        uint32_t sender=0;
        uint32_t capacity=frame->ecx;
        if (!capacity || capacity>IPC_MESSAGE_MAX ||
            !paging_user_range_valid(task_current_address_space(),
                                     frame->ebx,capacity,1)) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int length=ipc_receive(task_current_id(),message,capacity,&sender);
        if (length<0 || copy_to_user(frame->ebx,message,(uint32_t)length)<0) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        frame->eax=(uint32_t)length;
        frame->edx=sender;
        return 0;
    }

    if (frame->eax==5u) {
        frame->eax=task_current_id();
        return 0;
    }


    if (frame->eax==6u) {
        char path[VFS_PATH_MAX+1u];
        if(copy_user_string(path,frame->ebx,sizeof(path))<0) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int descriptor=process_file_open(path,frame->ecx);
        frame->eax=descriptor<0?SYSCALL_ERROR:(uint32_t)descriptor;
        return 0;
    }

    if (frame->eax==7u) {
        uint32_t count=frame->edx;
        uint8_t buffer[256];
        if(count>sizeof(buffer) ||
           (count && !paging_user_range_valid(task_current_address_space(),
                                              frame->ecx,count,1))) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int result=process_file_read((int)frame->ebx,buffer,count);
        if(result<0 || (uint32_t)result>count ||
           (result && copy_to_user(frame->ecx,buffer,(uint32_t)result)<0))
            frame->eax=SYSCALL_ERROR;
        else
            frame->eax=(uint32_t)result;
        return 0;
    }

    if (frame->eax==8u) {
        uint32_t count=frame->edx;
        uint8_t buffer[256];
        if(count>sizeof(buffer) ||
           (count && copy_from_user(buffer,frame->ecx,count)<0)) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int result=process_file_write((int)frame->ebx,buffer,count);
        frame->eax=result<0?SYSCALL_ERROR:(uint32_t)result;
        return 0;
    }

    if (frame->eax==9u) {
        int result=process_file_close((int)frame->ebx);
        frame->eax=result<0?SYSCALL_ERROR:0;
        return 0;
    }

    if (frame->eax==10u) {
        int result=process_file_seek((int)frame->ebx,(int32_t)frame->ecx,
                                     (int)frame->edx);
        frame->eax=result<0?SYSCALL_ERROR:0;
        return 0;
    }

    frame->eax=SYSCALL_ERROR;
    return 0;
}

int syscall_abort_user_exception(uint32_t *saved_frame, uint32_t vector,
                                 uint32_t error_code) {
    (void)error_code;
    if (!saved_frame || vector >= 32u || vector == 2u ||
        vector == 8u || vector == 18u)
        return 0;
    struct syscall_frame *frame=(struct syscall_frame *)(void *)saved_frame;
    if ((frame->cs & 3u) != 3u) return 0;
    return task_terminate_current(-(int)(vector+1u))==0;
}
