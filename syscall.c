#include "syscall.h"
#include "task.h"
#include "paging.h"
#include "ipc.h"
#include "console.h"
#include "vfs.h"
#include <stdint.h>

#define SYSCALL_ERROR 0xFFFFFFFFu
#define SYSCALL_WRITE_MAX 256u
#define SYSCALL_IO_MAX 256u

struct syscall_frame {
    uint32_t edi, esi, ebp, ignored_esp, ebx, edx, ecx, eax;
    uint32_t vector, error, eip, cs, eflags, user_esp, user_ss;
};

static void wipe_stack_buffer(void *buffer,uint32_t length) {
    volatile uint8_t *bytes=(volatile uint8_t *)buffer;
    for(uint32_t i=0;i<length;i++) bytes[i]=0;
}

static int copy_from_user(void *destination,uint32_t source,uint32_t length) {
    const struct paging_address_space *space=task_current_address_space();
    if (!destination || !paging_user_range_valid(space,source,length,0))
        return -1;
    const volatile uint8_t *input=(const volatile uint8_t *)(uintptr_t)source;
    uint8_t *output=(uint8_t *)destination;
    for (uint32_t i=0;i<length;i++) output[i]=input[i];
    return 0;
}

static int copy_user_string(char *destination,uint32_t source,uint32_t capacity) {
    if (!destination || !capacity || source>0xFFFFFFFFu-(capacity-1u)) return -1;
    for (uint32_t i=0;i<capacity;i++) {
        if (copy_from_user(&destination[i],source+i,1u)<0) return -1;
        if (!destination[i]) return 0;
    }
    destination[capacity-1u]=0;
    return -1;
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

    /* Write a bounded byte string from the caller's own mapped address space. */
    if (frame->eax==6u) {
        uint8_t output[SYSCALL_WRITE_MAX];
        uint32_t length=frame->edx;
        if (!length || length>sizeof(output) ||
            copy_from_user(output,frame->ebx,length)<0) {
            wipe_stack_buffer(output,sizeof(output));
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        for (uint32_t i=0;i<length;i++)
            console_put_char((char)output[i]);
        wipe_stack_buffer(output,sizeof(output));
        frame->eax=length;
        return 0;
    }

    if (frame->eax==7u) {
        char path[VFS_PATH_MAX+1u];
        if (copy_user_string(path,frame->ebx,sizeof(path))<0) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int handle=vfs_open(path,frame->ecx);
        frame->eax=handle<0?SYSCALL_ERROR:(uint32_t)handle;
        wipe_stack_buffer(path,sizeof(path));
        return 0;
    }

    if (frame->eax==8u) {
        frame->eax=vfs_close((int)frame->ebx)<0?SYSCALL_ERROR:0;
        return 0;
    }

    if (frame->eax==9u) {
        uint8_t input[SYSCALL_IO_MAX];
        uint32_t length=frame->edx;
        if (length>sizeof(input) ||
            (length && !paging_user_range_valid(task_current_address_space(),
                                                 frame->ecx,length,1))) {
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int count=vfs_read((int)frame->ebx,input,length);
        if (count<0 || (count && copy_to_user(frame->ecx,input,(uint32_t)count)<0)) {
            wipe_stack_buffer(input,sizeof(input));
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        wipe_stack_buffer(input,sizeof(input));
        frame->eax=(uint32_t)count;
        return 0;
    }

    if (frame->eax==10u) {
        uint8_t output[SYSCALL_IO_MAX];
        uint32_t length=frame->edx;
        if (length>sizeof(output) ||
            (length && copy_from_user(output,frame->ecx,length)<0)) {
            wipe_stack_buffer(output,sizeof(output));
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        int count=vfs_write((int)frame->ebx,output,length);
        wipe_stack_buffer(output,sizeof(output));
        frame->eax=count<0?SYSCALL_ERROR:(uint32_t)count;
        return 0;
    }

    if (frame->eax==11u) {
        frame->eax=vfs_seek((int)frame->ebx,(int32_t)frame->ecx,
                            (int)frame->edx)<0?SYSCALL_ERROR:0;
        return 0;
    }

    if (frame->eax==3u) {
        uint8_t message[IPC_MESSAGE_MAX];
        uint32_t length=frame->edx;
        int result=-1;
        if (length<=IPC_MESSAGE_MAX &&
            copy_from_user(message,frame->ecx,length)==0)
            result=ipc_send(task_current_id(),frame->ebx,message,length);
        wipe_stack_buffer(message,sizeof(message));
        frame->eax=result<0?SYSCALL_ERROR:0;
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
        if (length<0) {
            wipe_stack_buffer(message,sizeof(message));
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        if (copy_to_user(frame->ebx,message,(uint32_t)length)<0) {
            wipe_stack_buffer(message,sizeof(message));
            frame->eax=SYSCALL_ERROR;
            return 0;
        }
        wipe_stack_buffer(message,sizeof(message));
        frame->eax=(uint32_t)length;
        frame->edx=sender;
        return 0;
    }

    if (frame->eax==5u) {
        frame->eax=task_current_id();
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
