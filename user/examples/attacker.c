#include "breados/syscall.h"

static volatile uint32_t attacker_value;

void _start(void) {
    static const char message[]="x";
    static const char failed[]="FAIL: attacker could not send IPC to victim.\n";

    if(bread_task_id()!=4u) bread_exit(2);
    attacker_value=1u;

    /* The victim maps this virtual address to a different physical page. */
    volatile uint32_t *other_process_data=
        (volatile uint32_t *)(uintptr_t)0x40002000u;
    *other_process_data=0xDEADBEEFu;

    if(bread_send(3u,message,1u)<0) {
        (void)bread_write(failed,(uint32_t)(sizeof(failed)-1u));
        bread_exit(1);
    }
    bread_exit(0);
}
