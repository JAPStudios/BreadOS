#include "breados/syscall.h"

static volatile uint32_t victim_value=0x13579BDFu;

void _start(void) {
    static const char passed[]=
        "PASS: another user process could not modify victim memory.\n";
    static const char failed[]=
        "FAIL: another user process modified victim memory.\n";
    char message[1]={0};
    uint32_t sender=0;

    if(bread_task_id()!=3u) bread_exit(2);
    for(;;) {
        if(bread_receive(message,sizeof(message),&sender)>=0) break;
    }
    if(sender==4u && victim_value==0x13579BDFu) {
        (void)bread_write(passed,(uint32_t)(sizeof(passed)-1u));
        bread_exit(0);
    }
    (void)bread_write(failed,(uint32_t)(sizeof(failed)-1u));
    bread_exit(1);
}
