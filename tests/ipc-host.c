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
    runnable[5]=1;

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

    /* Removing one sender must preserve messages from every other sender. */
    assert(ipc_send(3,4,sent,4)==0);
    assert(ipc_send(5,4,sent,6)==0);
    assert(ipc_send(3,4,sent,8)==0);
    runnable[3]=0;
    ipc_remove_sender(3);
    assert(ipc_receive(4,received,sizeof(received),&sender)==6);
    assert(sender==5);
    for(uint32_t i=0;i<6;i++) assert(received[i]==sent[i]);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);


    /* Sender cleanup must preserve FIFO order when the ring has wrapped. */
    runnable[3]=1;
    ipc_reset_task(4);
    sent[0]=10;
    assert(ipc_send(3,4,sent,1)==0);
    sent[0]=20;
    assert(ipc_send(5,4,sent,1)==0);
    sent[0]=30;
    assert(ipc_send(3,4,sent,1)==0);
    sent[0]=40;
    assert(ipc_send(5,4,sent,1)==0);
    assert(ipc_receive(4,received,1,&sender)==1 && sender==3 && received[0]==10);
    assert(ipc_receive(4,received,1,&sender)==1 && sender==5 && received[0]==20);
    sent[0]=50;
    assert(ipc_send(3,4,sent,1)==0);
    sent[0]=60;
    assert(ipc_send(5,4,sent,1)==0);
    runnable[3]=0;
    ipc_remove_sender(3);
    assert(ipc_receive(4,received,1,&sender)==1 && sender==5 && received[0]==40);
    assert(ipc_receive(4,received,1,&sender)==1 && sender==5 && received[0]==60);
    assert(ipc_receive(4,received,1,&sender)==-1);

    /* Resetting a recipient must discard its queued messages. */
    runnable[3]=1;
    assert(ipc_send(3,4,sent,4)==0);
    ipc_reset_task(4);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);

    runnable[4]=0;
    assert(ipc_send(3,4,sent,4)==-1);
    assert(ipc_receive(4,received,sizeof(received),&sender)==-1);
    return 0;
}
