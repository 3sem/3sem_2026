#include "transfer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(void) {
    int queue_id = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (queue_id < 0) {
        perror("msgget");
        return EXIT_FAILURE;
    }
    struct msqid_ds info;
    int stat_result = msgctl(queue_id, IPC_STAT, &info);
    size_t message_limit = probeMessageLimit(queue_id);
    int remove_result = msgctl(queue_id, IPC_RMID, NULL);
    if (stat_result < 0 || message_limit == 0 || remove_result < 0) {
        perror("message queue probe");
        return EXIT_FAILURE;
    }
    long fifo_capacity = probeFifoCapacity();
    if (fifo_capacity <= 0) {
        perror("FIFO probe");
        return EXIT_FAILURE;
    }
    printf("message_limit=%zu queue_capacity=%lu fifo_capacity=%ld\n",
           message_limit, (unsigned long) info.msg_qbytes, fifo_capacity);
    return EXIT_SUCCESS;
}
