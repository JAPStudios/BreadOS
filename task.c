#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define TASK_COUNT 3u
#define TASK_STACK_SIZE 4096u
#define KERNEL_CODE_SELECTOR 0x08u
#define INITIAL_EFLAGS 0x202u

/*
 * interrupt stubs save this frame in memory order:
 * edi, esi, ebp, saved-esp, ebx, edx, ecx, eax, vector, error,
 * eip, cs, eflags.
 */
struct task {
    uint32_t *saved_stack;
    uint8_t started;
};

static struct task tasks[TASK_COUNT];
static uint8_t task_stacks[TASK_COUNT - 1u][TASK_STACK_SIZE]
    __attribute__((aligned(16)));
static uint32_t current_task;
static volatile uint32_t switches;
static volatile uint32_t worker_a_ticks;
static volatile uint32_t worker_b_ticks;

static void task_exit(void) {
    /* Kernel tasks must not return into an arbitrary address. */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static void worker_a(void) {
    for (;;) {
        ++worker_a_ticks;
        __asm__ volatile ("pause");
    }
}

static void worker_b(void) {
    for (;;) {
        ++worker_b_ticks;
        __asm__ volatile ("pause");
    }
}

static uint32_t *make_initial_stack(uint8_t *stack, void (*entry)(void)) {
    uint32_t *sp = (uint32_t *)(void *)(stack + TASK_STACK_SIZE);

    /*
     * Leave a return address below the synthetic interrupt frame. After
     * popa, vector/error removal, and iret, ESP has the normal C entry
     * alignment and a safe task_exit return target.
     */
    *--sp = (uint32_t)(uintptr_t)task_exit;
    *--sp = INITIAL_EFLAGS;
    *--sp = KERNEL_CODE_SELECTOR;
    *--sp = (uint32_t)(uintptr_t)entry;
    *--sp = 0;  /* error code */
    *--sp = 32; /* timer vector */
    *--sp = 0;  /* eax */
    *--sp = 0;  /* ecx */
    *--sp = 0;  /* edx */
    *--sp = 0;  /* ebx */
    *--sp = 0;  /* saved ESP placeholder (ignored by popa) */
    *--sp = 0;  /* ebp */
    *--sp = 0;  /* esi */
    *--sp = 0;  /* edi */
    return sp;
}

void tasking_init(void) {
    for (size_t i = 0; i < TASK_COUNT; ++i) {
        tasks[i].saved_stack = 0;
        tasks[i].started = 0;
    }

    /* Task 0 is kernel_main/shell; its first frame is captured on IRQ0. */
    tasks[0].started = 1;
    tasks[1].saved_stack = make_initial_stack(task_stacks[0], worker_a);
    tasks[1].started = 1;
    tasks[2].saved_stack = make_initial_stack(task_stacks[1], worker_b);
    tasks[2].started = 1;
    current_task = 0;
    switches = 0;
    worker_a_ticks = 0;
    worker_b_ticks = 0;
}

uint32_t *task_schedule(uint32_t *frame) {
    if (frame == 0)
        return frame;

    tasks[current_task].saved_stack = frame;

    uint32_t next = current_task;
    for (uint32_t attempt = 0; attempt < TASK_COUNT; ++attempt) {
        next = (next + 1u) % TASK_COUNT;
        if (tasks[next].started && tasks[next].saved_stack != 0)
            break;
    }

    if (next != current_task) {
        current_task = next;
        ++switches;
    }
    return tasks[current_task].saved_stack;
}

uint32_t task_switch_count(void) {
    return switches;
}
