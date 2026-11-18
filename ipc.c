#include "ipc.h"
#include "task.h"
#include <stddef.h>
#include <stdint.h>

struct ipc_message {
    uint32_t sender;
    uint8_t length;
    uint8_t data[IPC_MESSAGE_MAX];
};

struct ipc_mailbox {
    uint8_t head;
    uint8_t count;
    struct ipc_message messages[IPC_QUEUE_CAPACITY];
};

static struct ipc_mailbox mailboxes[TASK_MAX];

static void clear_message(struct ipc_message *message) {
    message->sender=0;
    message->length=0;
    volatile uint8_t *bytes=message->data;
    for (uint32_t i=0;i<IPC_MESSAGE_MAX;i++) bytes[i]=0;
}

void ipc_reset_task(uint32_t task_id) {
    if (task_id>=TASK_MAX) return;
    struct ipc_mailbox *box=&mailboxes[task_id];
    box->head=0;
    box->count=0;
    for (uint32_t i=0;i<IPC_QUEUE_CAPACITY;i++)
        clear_message(&box->messages[i]);
}

int ipc_send(uint32_t sender,uint32_t target,
             const void *data,uint32_t length) {
    if (sender<TASK_KERNEL_COUNT || sender>=TASK_MAX ||
        target<TASK_KERNEL_COUNT || target>=TASK_MAX ||
        !task_is_runnable(sender) || !task_is_runnable(target) ||
        !data || !length || length>IPC_MESSAGE_MAX)
        return -1;

    struct ipc_mailbox *box=&mailboxes[target];
    if (box->count>=IPC_QUEUE_CAPACITY) return -1;
    uint32_t index=(box->head+box->count)%IPC_QUEUE_CAPACITY;
    struct ipc_message *message=&box->messages[index];
    message->sender=sender;
    message->length=(uint8_t)length;
    const uint8_t *source=(const uint8_t *)data;
    for (uint32_t i=0;i<length;i++) message->data[i]=source[i];
    ++box->count;
    return 0;
}

int ipc_receive(uint32_t receiver,void *data,uint32_t capacity,
                uint32_t *sender_out) {
    if (receiver<TASK_KERNEL_COUNT || receiver>=TASK_MAX ||
        !task_is_runnable(receiver) || !data || !sender_out)
        return -1;

    struct ipc_mailbox *box=&mailboxes[receiver];
    if (!box->count) return -1;
    struct ipc_message *message=&box->messages[box->head];
    if (capacity<message->length) return -2;

    uint8_t *destination=(uint8_t *)data;
    for (uint32_t i=0;i<message->length;i++)
        destination[i]=message->data[i];
    *sender_out=message->sender;
    int length=message->length;
    clear_message(message);
    box->head=(uint8_t)((box->head+1u)%IPC_QUEUE_CAPACITY);
    --box->count;
    return length;
}

void ipc_remove_sender(uint32_t task_id) {
    if (task_id<TASK_KERNEL_COUNT || task_id>=TASK_MAX) return;
    /* A successful send owns a queue slot until the receiver consumes it.
       Preserve queued payloads when the sender exits, but clear its stale ID. */
    for(uint32_t target=TASK_KERNEL_COUNT;target<TASK_MAX;target++) {
        struct ipc_mailbox *box=&mailboxes[target];
        for(uint32_t i=0;i<box->count;i++) {
            uint32_t index=(box->head+i)%IPC_QUEUE_CAPACITY;
            if(box->messages[index].sender==task_id)
                box->messages[index].sender=0;
        }
    }
}
