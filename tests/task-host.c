#include <assert.h>
#include <stdint.h>
#include "task.h"

static uint32_t active_directory;
static uint32_t kernel_switches;
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

    tasking_init();
    assert(task_switch_count() == 0);
    assert(active_directory == 0);
    struct paging_address_space user_space = {0x1000u, 0x2000u};
    assert(task_set_address_space(&user_space) == 0);

    /* First timer tick saves the shell frame and starts worker A. */
    uint32_t *a = task_schedule(shell_frame);
    assert(a != 0);
    assert(a != shell_frame);
    assert(task_switch_count() == 1);

    /* Next tick saves A and starts worker B on a distinct initial stack. */
    uint32_t *b = task_schedule(worker_a_frame);
    assert(b != 0);
    assert(b != a);
    assert(b != worker_a_frame);
    assert(task_switch_count() == 2);

    /* Next tick saves B and resumes the original shell interrupt frame. */
    uint32_t *shell = task_schedule(worker_b_frame);
    assert(shell == shell_frame);
    assert(task_switch_count() == 3);
    assert(active_directory == user_space.directory_physical);

    /* A later round resumes the exact saved worker frames. */
    assert(task_schedule(shell_frame) == worker_a_frame);
    assert(active_directory == 0); /* Worker A must not inherit user mappings. */
    assert(task_schedule(worker_a_frame) == worker_b_frame);
    assert(task_schedule(worker_b_frame) == shell_frame);
    assert(active_directory == user_space.directory_physical);
    assert(task_set_address_space(0) == 0);
    assert(task_switch_count() == 6);

    /* A null frame is rejected without advancing scheduler state. */
    assert(task_schedule(0) == 0);
    assert(task_switch_count() == 6);
    return 0;
}
