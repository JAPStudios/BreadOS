#include <assert.h>
#include <stdint.h>
#include "ipc.h"
#include "task.h"

static int runnable[TASK_MAX];
int task_is_runnable(uint32_t id) {
    return id<TASK_MAX && runnable[id];
}

int main(void) {
    uint8_t sent[IPC_MESSAGE_MAX];
    uint8_t received[IPC_MESSAGE_MAX];
    uint32_t sender=0;
    for(uint32_t i=0;i<sizeof(sent);i++) sent[i]=(uint8_t)(i+1u);
    runnable[3]=1;
    runnable[4]=1;

    assert(ipc_send(3,4,sent,IPC_MESSAGE_MAX)==0);
    assert(ipc_receive(4,received,IPC_MESSAGE_MAX,&sender)==IPC_MESSAGE_MAX);
    assert(sender==3);
    for(uint32_t i=0;i<sizeof(sent);i++) assert(sent[i]==received[i]);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);
    assert(ipc_send(3,4,sent,0)==-1);
    assert(ipc_send(3,4,sent,IPC_MESSAGE_MAX+1u)==-1);

    for(uint32_t i=0;i<IPC_QUEUE_CAPACITY;i++)
        assert(ipc_send(3,4,sent,4)==0);
    assert(ipc_send(3,4,sent,4)==-1);
    assert(ipc_receive(4,received,3,&sender)==-2);
    assert(ipc_receive(4,received,4,&sender)==4);
    assert(sender==3);

    ipc_reset_task(4);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);
    runnable[4]=0;
    assert(ipc_send(3,4,sent,4)==-1);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);
    return 0;
}
