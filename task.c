#include "task.h"
#include <stddef.h>
#include <stdint.h>

#define TASK_COUNT 3u
#define TASK_STACK_SIZE 4096u
#define KERNEL_CODE_SELECTOR 0x08u
#define INITIAL_EFLAGS 0x202u

/*
 * The interrupt assembly saves this exact stack layout:
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
static uint32_t switches;

static volatile uint32_t worker_a_ticks;
static volatile uint32_t worker_b_ticks;

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

    /* iret frame, then the synthetic vector/error pair and pusha frame. */
    *--sp = INITIAL_EFLAGS;
    *--sp = KERNEL_CODE_SELECTOR;
    *--sp = (uint32_t)(uintptr_t)entry;
    *--sp = 0;  /* error code */
    *--sp = 32; /* timer vector */
    *--sp = 0;  /* eax */
    *--sp = 0;  /* ecx */
    *--sp = 0;  /* edx */
    *--sp = 0;  /* ebx */
    *--sp = 0;  /* saved esp placeholder */
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

    /* Task 0 is the already-running kernel_main/shell context. Its first
       interrupt frame is captured by task_schedule(). */
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
