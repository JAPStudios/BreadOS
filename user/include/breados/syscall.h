#ifndef BREADOS_USER_SYSCALL_H
#define BREADOS_USER_SYSCALL_H

#include <stdint.h>

/* BreadOS i386 user-mode system-call ABI. Errors are returned as -1. */
#define BREAD_O_READ      0x01u
#define BREAD_O_WRITE     0x02u
#define BREAD_O_CREATE    0x04u
#define BREAD_O_TRUNCATE  0x08u
#define BREAD_O_BINARY    0x10u

static inline uint32_t bread_syscall0(uint32_t number) {
    uint32_t result=number;
    __asm__ volatile ("int $0x80" : "+a"(result) : : "memory","cc");
    return result;
}

static inline uint32_t bread_syscall1(uint32_t number,uint32_t ebx) {
    uint32_t result=number;
    __asm__ volatile ("int $0x80" : "+a"(result) : "b"(ebx) : "memory","cc");
    return result;
}

static inline uint32_t bread_syscall2(uint32_t number,uint32_t ebx,uint32_t ecx) {
    uint32_t result=number;
    __asm__ volatile ("int $0x80" : "+a"(result) : "b"(ebx),"c"(ecx) : "memory","cc");
    return result;
}

static inline uint32_t bread_syscall3(uint32_t number,uint32_t ebx,
                                      uint32_t ecx,uint32_t edx) {
    uint32_t result=number;
    __asm__ volatile ("int $0x80" : "+a"(result)
                      : "b"(ebx),"c"(ecx),"d"(edx) : "memory","cc");
    return result;
}

static inline int32_t bread_write_char(uint8_t character) {
    return (int32_t)bread_syscall1(1u,(uint32_t)character);
}

static inline int32_t bread_write(const void *buffer,uint32_t length) {
    return (int32_t)bread_syscall3(6u, (uint32_t)(uintptr_t)buffer,0u,length);
}

static inline __attribute__((noreturn)) void bread_exit(int32_t status) {
    (void)bread_syscall1(2u,(uint32_t)status);
    __builtin_unreachable();
}

static inline int32_t bread_send(uint32_t target,const void *message,uint32_t length) {
    return (int32_t)bread_syscall3(3u,target,(uint32_t)(uintptr_t)message,length);
}

static inline int32_t bread_receive(void *buffer,uint32_t capacity,uint32_t *sender) {
    uint32_t result=4u,source=0;
    __asm__ volatile ("int $0x80" : "+a"(result),"=d"(source)
                      : "b"((uint32_t)(uintptr_t)buffer),"c"(capacity)
                      : "memory","cc");
    if (sender) *sender=source;
    return (int32_t)result;
}

static inline uint32_t bread_task_id(void) {
    return bread_syscall0(5u);
}

static inline int32_t bread_open(const char *path,uint32_t flags) {
    return (int32_t)bread_syscall2(7u,(uint32_t)(uintptr_t)path,flags);
}

static inline int32_t bread_close(int32_t descriptor) {
    return (int32_t)bread_syscall1(8u,(uint32_t)descriptor);
}

static inline int32_t bread_read(int32_t descriptor,void *buffer,uint32_t length) {
    return (int32_t)bread_syscall3(9u,(uint32_t)descriptor,
                                   (uint32_t)(uintptr_t)buffer,length);
}

static inline int32_t bread_file_write(int32_t descriptor,const void *buffer,
                                       uint32_t length) {
    return (int32_t)bread_syscall3(10u,(uint32_t)descriptor,
                                   (uint32_t)(uintptr_t)buffer,length);
}

static inline int32_t bread_seek(int32_t descriptor,int32_t offset,int32_t origin) {
    return (int32_t)bread_syscall3(11u,(uint32_t)descriptor,
                                   (uint32_t)offset,(uint32_t)origin);
}

#endif
