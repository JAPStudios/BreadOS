#include "task.h"
#include "paging.h"
#include "gdt.h"
#include <stddef.h>
#include <stdint.h>

#define TASK_STACK_SIZE 4096u
#define KERNEL_CODE_SELECTOR 0x08u
#define INITIAL_EFLAGS 0x202u

struct task {
    uint32_t *saved_stack;
    enum task_state state;
    struct paging_address_space address_space;
    uint32_t kernel_stack_top;
    int exit_code;
};

static struct task tasks[TASK_MAX];
static uint8_t task_stacks[TASK_KERNEL_COUNT - 1u][TASK_STACK_SIZE]
    __attribute__((aligned(16)));
static uint32_t current_task;
static volatile uint32_t switches;
static volatile uint32_t worker_a_ticks;
static volatile uint32_t worker_b_ticks;

static void task_exit(void) {
    for (;;) __asm__ volatile ("cli; hlt");
}

static void worker_a(void) {
    for (;;) {
        ++worker_a_ticks;
        __asm__ volatile ("hlt" ::: "memory");
    }
}

static void worker_b(void) {
    for (;;) {
        ++worker_b_ticks;
        __asm__ volatile ("hlt" ::: "memory");
    }
}

static uint32_t *make_initial_kernel_stack(uint8_t *stack, void (*entry)(void)) {
    uint32_t *sp = (uint32_t *)(void *)(stack + TASK_STACK_SIZE);
    *--sp = (uint32_t)(uintptr_t)task_exit;
    *--sp = INITIAL_EFLAGS;
    *--sp = KERNEL_CODE_SELECTOR;
    *--sp = (uint32_t)(uintptr_t)entry;
    *--sp = 0;
    *--sp = 32;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    return sp;
}

void tasking_init(void) {
    for (size_t i = 0; i < TASK_MAX; ++i) {
        tasks[i].saved_stack = 0;
        tasks[i].state = TASK_UNUSED;
        tasks[i].address_space.directory_physical = 0;
        tasks[i].address_space.user_table_physical = 0;
        tasks[i].kernel_stack_top = 0;
        tasks[i].exit_code = 0;
    }

    tasks[0].state = TASK_RUNNABLE;
    tasks[1].saved_stack = make_initial_kernel_stack(task_stacks[0], worker_a);
    tasks[1].state = TASK_RUNNABLE;
    tasks[1].kernel_stack_top =
        (uint32_t)(uintptr_t)(task_stacks[0] + TASK_STACK_SIZE);
    tasks[2].saved_stack = make_initial_kernel_stack(task_stacks[1], worker_b);
    tasks[2].state = TASK_RUNNABLE;
    tasks[2].kernel_stack_top =
        (uint32_t)(uintptr_t)(task_stacks[1] + TASK_STACK_SIZE);
    current_task = 0;
    switches = 0;
    worker_a_ticks = 0;
    worker_b_ticks = 0;
}

int task_set_kernel_stack(uint32_t stack_top) {
    if (!stack_top || tasks[current_task].state != TASK_RUNNABLE) return -1;
    tasks[current_task].kernel_stack_top = stack_top;
    gdt_set_kernel_stack(stack_top);
    return 0;
}

int task_set_address_space(const struct paging_address_space *space) {
    if (tasks[current_task].state != TASK_RUNNABLE) return -1;
    if (space && (!space->directory_physical || !space->user_table_physical))
        return -1;
    if (space) tasks[current_task].address_space = *space;
    else {
        tasks[current_task].address_space.directory_physical = 0;
        tasks[current_task].address_space.user_table_physical = 0;
    }
    return 0;
}

int task_create_user(uint32_t *initial_frame,
                     const struct paging_address_space *space,
                     uint32_t kernel_stack_top) {
    if (!initial_frame || !space || !space->directory_physical ||
        !space->user_table_physical || !kernel_stack_top)
        return -1;
    for (uint32_t i = TASK_KERNEL_COUNT; i < TASK_MAX; ++i) {
        if (tasks[i].state != TASK_UNUSED) continue;
        tasks[i].saved_stack = initial_frame;
        tasks[i].address_space = *space;
        tasks[i].kernel_stack_top = kernel_stack_top;
        tasks[i].exit_code = 0;
        /* Publish the runnable state only after the complete context is ready. */
        __asm__ volatile ("" : : : "memory");
        tasks[i].state = TASK_RUNNABLE;
        return (int)i;
    }
    return -1;
}

int task_terminate_current(int exit_code) {
    if (current_task < TASK_KERNEL_COUNT ||
        tasks[current_task].state != TASK_RUNNABLE)
        return -1;
    tasks[current_task].exit_code = exit_code;
    __asm__ volatile ("" : : : "memory");
    tasks[current_task].state = TASK_ZOMBIE;
    return 0;
}

int task_is_zombie(uint32_t task_id) {
    return task_id < TASK_MAX && tasks[task_id].state == TASK_ZOMBIE;
}

int task_exit_code(uint32_t task_id) {
    if (task_id >= TASK_MAX || tasks[task_id].state != TASK_ZOMBIE) return -1;
    return tasks[task_id].exit_code;
}

int task_destroy(uint32_t task_id) {
    if (task_id < TASK_KERNEL_COUNT || task_id >= TASK_MAX ||
        tasks[task_id].state != TASK_ZOMBIE || task_id == current_task)
        return -1;
    tasks[task_id].saved_stack = 0;
    tasks[task_id].address_space.directory_physical = 0;
    tasks[task_id].address_space.user_table_physical = 0;
    tasks[task_id].kernel_stack_top = 0;
    tasks[task_id].exit_code = 0;
    __asm__ volatile ("" : : : "memory");
    tasks[task_id].state = TASK_UNUSED;
    return 0;
}

uint32_t task_current_id(void) {
    return current_task;
}

uint32_t *task_schedule(uint32_t *frame) {
    if (!frame) return frame;
    if (tasks[current_task].state == TASK_RUNNABLE)
        tasks[current_task].saved_stack = frame;

    uint32_t next = TASK_MAX;
    for (uint32_t attempt = 1; attempt <= TASK_MAX; ++attempt) {
        uint32_t candidate = (current_task + attempt) % TASK_MAX;
        if (tasks[candidate].state != TASK_RUNNABLE ||
            !tasks[candidate].saved_stack)
            continue;

        if (tasks[candidate].address_space.directory_physical) {
            if (paging_switch_address_space(&tasks[candidate].address_space) < 0) {
                tasks[candidate].exit_code = -256;
                tasks[candidate].state = TASK_ZOMBIE;
                continue;
            }
        } else {
            paging_switch_kernel_address_space();
        }
        next = candidate;
        break;
    }

    /* The shell task is permanently runnable and provides the idle fallback. */
    if (next == TASK_MAX) {
        next = 0;
        paging_switch_kernel_address_space();
    }

    if (next != current_task) {
        current_task = next;
        ++switches;
    }
    if (tasks[current_task].kernel_stack_top)
        gdt_set_kernel_stack(tasks[current_task].kernel_stack_top);
    return tasks[current_task].saved_stack;
}


uint32_t task_switch_count(void) {
    return switches;
}
