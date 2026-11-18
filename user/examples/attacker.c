#include "breados/syscall.h"

static volatile uint32_t attacker_value;

void _start(void) {
    static const char message[]="x";
    static const char wrong_id[]="FAIL: attacker task ID was not 4.\n";
    static const char failed[]="FAIL: attacker could not send IPC to victim.\n";
    uint32_t id=bread_task_id();

    (void)bread_write("attacker task id: ",18u);
    (void)bread_write_char((uint8_t)('0'+id));
    (void)bread_write_char('\n');
    if(id!=4u) {
        (void)bread_write(wrong_id,(uint32_t)(sizeof(wrong_id)-1u));
        bread_exit(2);
    }
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
