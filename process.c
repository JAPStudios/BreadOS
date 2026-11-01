#include "process.h"
#include "memory.h"
#include "paging.h"
#include "syscall.h"
#include <stdint.h>

#define USER_CODE_ADDRESS 0x00400000u
#define USER_STACK_ADDRESS 0x00401000u
#define PAGE_SIZE 4096u

extern void user_enter(uint32_t entry, uint32_t user_stack);

static void zero_page(uint32_t physical) {
    volatile uint32_t *p=(volatile uint32_t *)(uintptr_t)physical;
    for (uint32_t i=0;i<PAGE_SIZE/sizeof(uint32_t);i++) p[i]=0;
}

int process_run_user_demo(void) {
    static const uint8_t program[] = {
        0xB9,0x00,0x2D,0x31,0x01,       /* mov ecx, 20,000,000: allow timer preemption */
        0x49,0x75,0xFD,                 /* dec ecx; jnz */
        0xB8,0x01,0x00,0x00,0x00,       /* mov eax, 1: write character */
        0xBB,'U',0x00,0x00,0x00,       /* mov ebx, 'U' */
        0xCD,0x80,                     /* int 0x80 */
        0xB8,0x02,0x00,0x00,0x00,       /* mov eax, 2: exit */
        0xBB,0x00,0x00,0x00,0x00,       /* mov ebx, 0 */
        0xCD,0x80,                     /* int 0x80 */
        0xEB,0xFE                      /* defensive infinite loop */
    };
    uint32_t code_frame=memory_alloc_frame();
    if (!code_frame) return -1;
    uint32_t stack_frame=memory_alloc_frame();
    if (!stack_frame) { memory_free_frame(code_frame); return -1; }
    zero_page(code_frame);
    zero_page(stack_frame);
    volatile uint8_t *code=(volatile uint8_t *)(uintptr_t)code_frame;
    for (uint32_t i=0;i<sizeof(program);i++) code[i]=program[i];

    if (paging_map_user_page(USER_CODE_ADDRESS,code_frame,0)<0) {
        memory_free_frame(stack_frame);
        memory_free_frame(code_frame);
        return -1;
    }
    if (paging_map_user_page(USER_STACK_ADDRESS,stack_frame,1)<0) {
        paging_unmap_user_page(USER_CODE_ADDRESS);
        memory_free_frame(stack_frame);
        memory_free_frame(code_frame);
        return -1;
    }

    user_enter(USER_CODE_ADDRESS,USER_STACK_ADDRESS+PAGE_SIZE-16u);
    int exit_code=syscall_last_exit_code();
    paging_unmap_user_page(USER_STACK_ADDRESS);
    paging_unmap_user_page(USER_CODE_ADDRESS);
    memory_free_frame(stack_frame);
    memory_free_frame(code_frame);
    return exit_code;
}
