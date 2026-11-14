#include "process.h"
#include "memory.h"
#include "paging.h"
#include "syscall.h"
#include "elf_loader.h"
#include <stddef.h>
#include <stdint.h>

#define USER_CODE_ADDRESS 0x40000000u
#define USER_STACK_ADDRESS 0x40001000u
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

    struct paging_address_space address_space = {0, 0};
    uint32_t code_frame=memory_alloc_frame();
    if (!code_frame) return -1;
    uint32_t stack_frame=memory_alloc_frame();
    if (!stack_frame) {
        memory_free_frame(code_frame);
        return -1;
    }
    if (paging_address_space_create(&address_space)<0) {
        memory_free_frame(stack_frame);
        memory_free_frame(code_frame);
        return -1;
    }

    zero_page(code_frame);
    zero_page(stack_frame);
    volatile uint8_t *code=(volatile uint8_t *)(uintptr_t)code_frame;
    for (uint32_t i=0;i<sizeof(program);i++) code[i]=program[i];

    if (paging_address_space_map_user_page(&address_space,USER_CODE_ADDRESS,
                                           code_frame,0)<0 ||
        paging_address_space_map_user_page(&address_space,USER_STACK_ADDRESS,
                                           stack_frame,1)<0) {
        paging_address_space_destroy(&address_space);
        memory_free_frame(stack_frame);
        memory_free_frame(code_frame);
        return -1;
    }

    if (paging_switch_address_space(&address_space)<0) {
        paging_address_space_destroy(&address_space);
        memory_free_frame(stack_frame);
        memory_free_frame(code_frame);
        return -1;
    }

    user_enter(USER_CODE_ADDRESS,USER_STACK_ADDRESS+PAGE_SIZE-16u);
    paging_switch_kernel_address_space();

    int exit_code=syscall_last_exit_code();
    paging_address_space_unmap_user_page(&address_space,USER_STACK_ADDRESS);
    paging_address_space_unmap_user_page(&address_space,USER_CODE_ADDRESS);
    paging_address_space_destroy(&address_space);
    memory_free_frame(stack_frame);
    memory_free_frame(code_frame);
    return exit_code;
}


#define PROCESS_MAX_USER_PAGES 1024u
struct process_page {
    uint32_t virtual_address;
    uint32_t physical_address;
    uint8_t mapped;
};
static struct process_page process_pages[PROCESS_MAX_USER_PAGES];
static uint32_t process_page_count;

static void process_release_pages(struct paging_address_space *space) {
    for (uint32_t i=0;i<process_page_count;i++) {
        if (process_pages[i].mapped)
            paging_address_space_unmap_user_page(space,process_pages[i].virtual_address);
    }
    if (space->directory_physical) paging_address_space_destroy(space);
    for (uint32_t i=0;i<process_page_count;i++)
        memory_free_frame(process_pages[i].physical_address);
    process_page_count=0;
}

static int process_add_page(struct paging_address_space *space,
                            uint32_t virtual_address,int writable,
                            uint32_t *physical_out) {
    if (process_page_count>=PROCESS_MAX_USER_PAGES || !physical_out) return -1;
    uint32_t physical=memory_alloc_frame();
    if (!physical) return -1;
    zero_page(physical);
    struct process_page *record=&process_pages[process_page_count++];
    record->virtual_address=virtual_address;
    record->physical_address=physical;
    record->mapped=0;
    if (paging_address_space_map_user_page(space,virtual_address,physical,writable)<0)
        return -1;
    record->mapped=1;
    *physical_out=physical;
    return 0;
}

int process_execute_elf(const void *bytes,size_t size) {
    struct elf32_image image;
    if (elf32_inspect(bytes,size,&image)<0) return -1;

    struct paging_address_space space={0,0};
    process_page_count=0;
    if (paging_address_space_create(&space)<0) return -1;

    for (uint16_t i=0;i<image.program_header_count;i++) {
        struct elf32_load_segment segment;
        int result=elf32_load_segment_at(&image,i,&segment);
        if (result==1) continue;
        if (result<0) {
            process_release_pages(&space);
            return -1;
        }
        for (uint32_t offset=0;offset<segment.memory_size;offset+=PAGE_SIZE) {
            uint32_t physical;
            uint32_t virtual_address=segment.virtual_address+offset;
            if (process_add_page(&space,virtual_address,
                                 (segment.flags&2u)!=0,&physical)<0) {
                process_release_pages(&space);
                return -1;
            }
            if (offset<segment.file_size) {
                uint32_t remaining=segment.file_size-offset;
                uint32_t count=remaining>PAGE_SIZE?PAGE_SIZE:remaining;
                volatile uint8_t *destination=(volatile uint8_t *)(uintptr_t)physical;
                const uint8_t *source=image.bytes+segment.offset+offset;
                for (uint32_t j=0;j<count;j++) destination[j]=source[j];
            }
        }
    }

    uint32_t stack_frame;
    if (process_add_page(&space,ELF32_USER_STACK_PAGE,1,&stack_frame)<0) {
        process_release_pages(&space);
        return -1;
    }
    (void)stack_frame;

    if (paging_switch_address_space(&space)<0) {
        process_release_pages(&space);
        return -1;
    }
    syscall_begin_process();
    user_enter(image.entry,ELF32_USER_STACK_PAGE+PAGE_SIZE-16u);
    paging_switch_kernel_address_space();
    int exit_code=syscall_last_exit_code();
    process_release_pages(&space);
    return exit_code;
}
