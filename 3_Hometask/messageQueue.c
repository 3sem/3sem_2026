#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>

typedef struct file_message_t {
    long type;
    unsigned char data[];
} file_message_t;

int createQueue(size_t bufferSize) {
    struct msginfo limits;
    if (msgctl(0, IPC_INFO, (struct msqid_ds*)&limits) == -1) {
        perror("msgctl");
        return -1;
    }
    if (bufferSize > (size_t)limits.msgmax || bufferSize > (size_t)limits.msgmnb) {
        fprintf(stderr, "Message too large: msgmax=%d, msgmnb=%d bytes\n",
                limits.msgmax, limits.msgmnb);
        return -1;
    }

    int queueId = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (queueId == -1)
        perror("msgget");

    return queueId;
}

int sendMessage(int queueId, file_message_t* message, size_t dataSize) {
    int result = 0;
    do {
        result = msgsnd(queueId, message, dataSize, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1)
        perror("msgsnd");

    return result;
}

int sendFile(int queueId, file_message_t* message, size_t bufferSize) {
    message->type = 1;
    ssize_t dataSize = 0;

    do {
        dataSize = readChunk(STDIN_FILENO, message->data, bufferSize);
        if (dataSize == -1 || sendMessage(queueId, message, (size_t)dataSize) == -1)
            return -1;
    } while (dataSize > 0);

    return 0;
}

int receiveFile(int queueId, file_message_t* message, size_t bufferSize) {
    while (1) {
        ssize_t dataSize;
        do {
            dataSize = msgrcv(queueId, message, bufferSize, 1, 0);
        } while (dataSize == -1 && errno == EINTR);

        if (dataSize == -1) {
            perror("msgrcv");
            return -1;
        }

        if (dataSize == 0)
            return 0;

        if (writeFull(STDOUT_FILENO, message->data, (size_t)dataSize) == -1)
            return -1;
    }
}

int transferFile(size_t bufferSize) {
    int queueId = createQueue(bufferSize);
    if (queueId == -1)
        return -1;
    file_message_t* message = calloc(1, sizeof(*message) + bufferSize);
    if (!message) {
        perror("calloc");
        msgctl(queueId, IPC_RMID, NULL);
        return -1;
    }

    pid_t childPid = fork();
    if (childPid == -1) {
        perror("fork");
        msgctl(queueId, IPC_RMID, NULL);
        free(message);
        return -1;
    }

    if (childPid == 0) {
        int result = receiveFile(queueId, message, bufferSize);
        if (result == -1)
            msgctl(queueId, IPC_RMID, NULL);
        free(message);
        _exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int sendResult = sendFile(queueId, message, bufferSize);
    if (sendResult == -1)
        msgctl(queueId, IPC_RMID, NULL);
    int receiveResult = waitChild(childPid);

    msgctl(queueId, IPC_RMID, NULL);
    free(message);

    return sendResult == -1 || receiveResult == -1 ? -1 : 0;
}
