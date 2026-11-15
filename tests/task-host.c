#include <assert.h>
#include <stdint.h>
#include "task.h"
void ipc_reset_task(uint32_t task_id) { (void)task_id; }
void ipc_remove_sender(uint32_t task_id) { (void)task_id; }

static uint32_t active_directory;
static uint32_t kernel_switches;
static uint32_t active_kernel_stack;
void gdt_set_kernel_stack(uint32_t stack_top) { active_kernel_stack = stack_top; }
int paging_switch_address_space(const struct paging_address_space *space) {
    if (!space || !space->directory_physical || !space->user_table_physical) return -1;
    active_directory = space->directory_physical;
    return 0;
}
void paging_switch_kernel_address_space(void) {
    active_directory = 0;
    ++kernel_switches;
}

int main(void) {
    uint32_t shell_frame[16] = {0};
    uint32_t worker_a_frame[16] = {0};
    uint32_t worker_b_frame[16] = {0};
    uint32_t user_frame[16] = {0};

    tasking_init();
    assert(task_switch_count() == 0);
    assert(active_directory == 0);
    assert(task_set_kernel_stack(0x3000u) == 0);
    assert(active_kernel_stack == 0x3000u);
    struct paging_address_space user_space = {0x1000u, 0x2000u};
    assert(task_set_address_space(&user_space) == 0);

    uint32_t *a = task_schedule(shell_frame);
    assert(a != 0 && a != shell_frame);
    assert(task_switch_count() == 1);
    uint32_t worker_a_stack = active_kernel_stack;
    assert(worker_a_stack != 0 && worker_a_stack != 0x3000u);

    uint32_t *b = task_schedule(worker_a_frame);
    assert(b != 0 && b != a && b != worker_a_frame);
    assert(task_switch_count() == 2);
    uint32_t worker_b_stack = active_kernel_stack;
    assert(worker_b_stack != 0 && worker_b_stack != worker_a_stack);

    assert(task_schedule(worker_b_frame) == shell_frame);
    assert(task_switch_count() == 3);
    assert(active_kernel_stack == 0x3000u);
    assert(active_directory == user_space.directory_physical);

    assert(task_schedule(shell_frame) == worker_a_frame);
    assert(active_directory == 0);
    assert(active_kernel_stack == worker_a_stack);
    assert(task_schedule(worker_a_frame) == worker_b_frame);
    assert(active_kernel_stack == worker_b_stack);
    assert(task_schedule(worker_b_frame) == shell_frame);
    assert(active_kernel_stack == 0x3000u);
    assert(active_directory == user_space.directory_physical);
    assert(task_set_address_space(0) == 0);
    assert(task_switch_count() == 6);
    assert(kernel_switches == 4);

    int id = task_create_user(user_frame, &user_space, 0x5000u);
    assert(id == 3);
    assert(task_schedule(shell_frame) == worker_a_frame);
    assert(task_schedule(worker_a_frame) == worker_b_frame);
    assert(task_schedule(worker_b_frame) == user_frame);
    assert(task_current_id() == (uint32_t)id);
    assert(active_directory == user_space.directory_physical);
    assert(active_kernel_stack == 0x5000u);

    assert(task_terminate_current(9) == 0);
    assert(task_is_zombie((uint32_t)id));
    assert(task_exit_code((uint32_t)id) == 9);
    assert(task_schedule(user_frame) == shell_frame);
    assert(task_current_id() == 0);
    assert(active_directory == 0);
    assert(task_destroy((uint32_t)id) == 0);
    assert(!task_is_zombie((uint32_t)id));

    int reused = task_create_user(user_frame, &user_space, 0x6000u);
    assert(reused == id);
    assert(task_schedule(shell_frame) == worker_a_frame);
    assert(task_schedule(worker_a_frame) == worker_b_frame);
    assert(task_schedule(worker_b_frame) == user_frame);
    assert(task_terminate_current(-11) == 0);
    assert(task_schedule(user_frame) == shell_frame);
    assert(task_exit_code((uint32_t)reused) == -11);
    assert(task_destroy((uint32_t)reused) == 0);

    assert(task_schedule(0) == 0);
    return 0;
}
