#include "breados/syscall.h"

static volatile uint32_t victim_value=0x13579BDFu;

void _start(void) {
    static const char passed[]=
        "PASS: another user process could not modify victim memory.\n";
    static const char wrong_id[]="FAIL: victim task ID was not 3.\n";
    static const char wrong_sender[]="FAIL: victim received IPC from the wrong task.\n";
    static const char modified[]="FAIL: another user process modified victim memory.\n";
    char message[1]={0};
    uint32_t sender=0;
    uint32_t id=bread_task_id();

    (void)bread_write("victim task id: ",16u);
    (void)bread_write_char((uint8_t)('0'+id));
    (void)bread_write_char('\n');
    if(id!=3u) {
        (void)bread_write(wrong_id,(uint32_t)(sizeof(wrong_id)-1u));
        bread_exit(2);
    }
    for(;;) {
        if(bread_receive(message,sizeof(message),&sender)>=0) break;
    }
    (void)bread_write("victim received IPC.\\n",21u);
    if(sender!=4u) {
        (void)bread_write(wrong_sender,(uint32_t)(sizeof(wrong_sender)-1u));
        bread_exit(1);
    }
    if(victim_value==0x13579BDFu) {
        (void)bread_write(passed,(uint32_t)(sizeof(passed)-1u));
        bread_exit(0);
    }
    (void)bread_write(modified,(uint32_t)(sizeof(modified)-1u));
    bread_exit(1);
}
