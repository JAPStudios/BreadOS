#ifndef BREADOS_IPC_H
#define BREADOS_IPC_H

#include <stdint.h>

#define IPC_MESSAGE_MAX 64u
#define IPC_QUEUE_CAPACITY 8u

/* Nonblocking, bounded message passing between live user tasks. */
int ipc_send(uint32_t sender, uint32_t target,
             const void *data, uint32_t length);
int ipc_receive(uint32_t receiver, void *data, uint32_t capacity,
                uint32_t *sender_out);

/* Clear queued messages when a task slot is created or reclaimed. */
void ipc_reset_task(uint32_t task_id);

#endif
