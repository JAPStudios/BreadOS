#include "process.h"
#include "task.h"
#include "memory.h"
#include "paging.h"
#include "elf_loader.h"
#include "pfs.h"
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096u
#define PROCESS_CAPACITY (TASK_MAX - TASK_KERNEL_COUNT)
#define PROCESS_MAX_USER_PAGES 1024u
#define USER_CODE_SELECTOR 0x1Bu
#define USER_DATA_SELECTOR 0x23u
#define USER_INITIAL_EFLAGS 0x202u

struct process_page {
    uint32_t physical_address;
};

struct process_record {
    uint8_t used;
    uint32_t task_id;
    uint32_t kernel_stack_frame;
    struct paging_address_space address_space;
    uint32_t page_count;
    struct process_page pages[PROCESS_MAX_USER_PAGES];
};

static struct process_record processes[PROCESS_CAPACITY];

static void zero_page(uint32_t physical) {
    volatile uint32_t *p=(volatile uint32_t *)(uintptr_t)physical;
    for (uint32_t i=0;i<PAGE_SIZE/sizeof(uint32_t);i++) p[i]=0;
}

static void elf_put16(uint8_t *image,uint32_t offset,uint16_t value) {
    image[offset]=(uint8_t)value;
    image[offset+1]=(uint8_t)(value>>8);
}

static void elf_put32(uint8_t *image,uint32_t offset,uint32_t value) {
    for (uint32_t i=0;i<4;i++) image[offset+i]=(uint8_t)(value>>(i*8u));
}

static struct process_record *process_reserve(void) {
    for (uint32_t i=0;i<PROCESS_CAPACITY;i++) {
        if (!processes[i].used) {
            struct process_record *p=&processes[i];
            p->task_id=0;
            p->kernel_stack_frame=0;
            p->address_space.directory_physical=0;
            p->address_space.user_table_physical=0;
            p->page_count=0;
            __asm__ volatile ("" : : : "memory");
            p->used=1;
            return p;
        }
    }
    return 0;
}

static void process_release_resources(struct process_record *p) {
    if (!p) return;
    if (p->address_space.directory_physical)
        paging_address_space_destroy(&p->address_space);
    for (uint32_t i=0;i<p->page_count;i++)
        memory_free_frame(p->pages[i].physical_address);
    if (p->kernel_stack_frame)
        memory_free_frame(p->kernel_stack_frame);
    p->task_id=0;
    p->kernel_stack_frame=0;
    p->page_count=0;
    __asm__ volatile ("" : : : "memory");
    p->used=0;
}

static int process_add_page(struct process_record *p,uint32_t virtual_address,
                            int writable,uint32_t *physical_out) {
    if (!p || p->page_count>=PROCESS_MAX_USER_PAGES || !physical_out) return -1;
    uint32_t physical=memory_alloc_frame();
    if (!physical) return -1;
    zero_page(physical);
    if (paging_address_space_map_user_page(&p->address_space,virtual_address,
                                           physical,writable)<0) {
        memory_free_frame(physical);
        return -1;
    }
    p->pages[p->page_count++].physical_address=physical;
    *physical_out=physical;
    return 0;
}

/* Match interrupts.s: pusha frame, vector/error, then the five-word ring-3 iret frame. */
static uint32_t *process_initial_frame(uint32_t kernel_stack_top,
                                       uint32_t entry,uint32_t user_stack) {
    uint32_t *sp=(uint32_t *)(uintptr_t)kernel_stack_top;
    *--sp=USER_DATA_SELECTOR;
    *--sp=user_stack;
    *--sp=USER_INITIAL_EFLAGS;
    *--sp=USER_CODE_SELECTOR;
    *--sp=entry;
    *--sp=0;   /* error code */
    *--sp=32;  /* synthetic timer vector */
    *--sp=0;   /* eax */
    *--sp=0;   /* ecx */
    *--sp=0;   /* edx */
    *--sp=0;   /* ebx */
    *--sp=0;   /* saved ESP placeholder */
    *--sp=0;   /* ebp */
    *--sp=0;   /* esi */
    *--sp=0;   /* edi */
    return sp;
}

static int process_spawn_elf_inner(const void *bytes,size_t size) {
    struct elf32_image image;
    if (elf32_inspect(bytes,size,&image)<0) return -1;

    struct process_record *p=process_reserve();
    if (!p) return -1;
    if (paging_address_space_create(&p->address_space)<0) {
        process_release_resources(p);
        return -1;
    }

    for (uint16_t i=0;i<image.program_header_count;i++) {
        struct elf32_load_segment segment;
        int result=elf32_load_segment_at(&image,i,&segment);
        if (result==1) continue;
        if (result<0) {
            process_release_resources(p);
            return -1;
        }
        for (uint32_t offset=0;offset<segment.memory_size;offset+=PAGE_SIZE) {
            uint32_t physical;
            uint32_t virtual_address=segment.virtual_address+offset;
            if (process_add_page(p,virtual_address,
                                 (segment.flags&2u)!=0,&physical)<0) {
                process_release_resources(p);
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
    if (process_add_page(p,ELF32_USER_STACK_PAGE,1,&stack_frame)<0) {
        process_release_resources(p);
        return -1;
    }
    (void)stack_frame;

    p->kernel_stack_frame=memory_alloc_frame();
    if (!p->kernel_stack_frame) {
        process_release_resources(p);
        return -1;
    }
    zero_page(p->kernel_stack_frame);
    uint32_t stack_top=p->kernel_stack_frame+PAGE_SIZE;
    uint32_t *initial_frame=process_initial_frame(
        stack_top,image.entry,ELF32_USER_STACK_PAGE+PAGE_SIZE-16u);
    int task_id=task_create_user(initial_frame,&p->address_space,stack_top);
    if (task_id<0) {
        process_release_resources(p);
        return -1;
    }
    p->task_id=(uint32_t)task_id;
    return task_id;
}

int process_execute_elf(const void *bytes,size_t size) {
    return process_spawn_elf_inner(bytes,size);
}

int process_execute_file(const char *path) {
    static uint8_t executable[65535];
    if (!path || !pfs_ready()) return -1;
    int size=pfs_read_binary(path,executable,sizeof(executable));
    if (size<=0) return -1;
    return process_execute_elf(executable,(size_t)size);
}

void process_reap_zombies(void) {
    for (uint32_t i=0;i<PROCESS_CAPACITY;i++) {
        struct process_record *p=&processes[i];
        if (!p->used || !task_is_zombie(p->task_id)) continue;
        uint32_t id=p->task_id;
        process_release_resources(p);
        (void)task_destroy(id);
    }
}

uint32_t process_active_count(void) {
    uint32_t count=0;
    for (uint32_t i=0;i<PROCESS_CAPACITY;i++)
        if (processes[i].used) ++count;
    return count;
}

int process_run_user_demo(void) {
    static const uint8_t program[] = {
        0xB9,0x00,0x2D,0x31,0x01,
        0x49,0x75,0xFD,
        0xB8,0x01,0x00,0x00,0x00,
        0xBB,'U',0x00,0x00,0x00,
        0xCD,0x80,
        0xB8,0x02,0x00,0x00,0x00,
        0xBB,0x00,0x00,0x00,0x00,
        0xCD,0x80,
        0xEB,0xFE
    };
    static uint8_t executable[8192];
    for (uint32_t i=0;i<sizeof(executable);i++) executable[i]=0;

    executable[0]=0x7F;executable[1]='E';executable[2]='L';executable[3]='F';
    executable[4]=1;executable[5]=1;executable[6]=1;
    elf_put16(executable,16,2);
    elf_put16(executable,18,3);
    elf_put32(executable,20,1);
    elf_put32(executable,24,ELF32_USER_START);
    elf_put32(executable,28,52);
    elf_put16(executable,40,52);
    elf_put16(executable,42,32);
    elf_put16(executable,44,1);
    elf_put32(executable,52,1);
    elf_put32(executable,56,4096);
    elf_put32(executable,60,ELF32_USER_START);
    elf_put32(executable,68,(uint32_t)sizeof(program));
    elf_put32(executable,72,PAGE_SIZE);
    elf_put32(executable,76,5);
    elf_put32(executable,80,PAGE_SIZE);
    for (uint32_t i=0;i<sizeof(program);i++) executable[4096u+i]=program[i];

    return process_execute_elf(executable,sizeof(executable));
}

int process_task_at(uint32_t index,uint32_t *task_id,int *exiting) {
    if (!task_id || !exiting) return -1;
    for (uint32_t i=0;i<PROCESS_CAPACITY;i++) {
        struct process_record *p=&processes[i];
        if (!p->used) continue;
        if (index) { --index; continue; }
        *task_id=p->task_id;
        *exiting=task_is_zombie(p->task_id);
        return 0;
    }
    return -1;
}
