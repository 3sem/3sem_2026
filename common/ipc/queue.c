#include "queue.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MESSAGE_TYPE 333

struct Message {
    long mtype;
    size_t size;
    int isLast;
    char data[];
};

/* msgsz excludes mtype but includes the header and any alignment padding. */
static const size_t HEADER_SIZE = offsetof(struct Message, data) - sizeof(long);

static int getQueue(key_t key, int flags){
    int id = msgget(key, flags);
    if(id == -1) perror("msgget");

    return id;
}

static int removeQueue(int id){
    int result = msgctl(id, IPC_RMID, NULL);
    if(result == -1 && errno != EIDRM && errno != EINVAL){
        perror("msgctl IPC_RMID");
        return -1;
    }
    return 0;
}

static struct Message* createMessage(size_t chunkSize){
    if(chunkSize == 0 || chunkSize > SIZE_MAX - sizeof(struct Message)){
        fprintf(stderr, "Invalid queue chunk size\n");
        return NULL;
    }

    struct Message* msg = calloc(1, sizeof(struct Message) + chunkSize);
    if(msg == NULL) perror("Queue message allocation");
    return msg;
}

static int checkQueueCapacity(int id, size_t chunkSize){
    FILE* limits = fopen("/proc/sys/kernel/msgmax", "r");
    if(limits == NULL){
        perror("open msgmax");
        return -1;
    }

    size_t maxMessage = 0;
    int parsed = fscanf(limits, "%zu", &maxMessage);
    int closed = fclose(limits);
    if(parsed != 1 || closed != 0){
        fprintf(stderr, "Cannot read kernel.msgmax\n");
        return -1;
    }

    struct msqid_ds info = {0};
    if(msgctl(id, IPC_STAT, &info) == -1){
        perror("msgctl IPC_STAT");
        return -1;
    }

    size_t messageSize = HEADER_SIZE + chunkSize;
    if(messageSize > maxMessage || messageSize > info.msg_qbytes){
        fprintf(stderr, "Queue chunk %zu needs %zu bytes including header; "
                        "msgmax=%zu, msg_qbytes=%zu. Configure limits before creating the queue.\n",
                chunkSize, messageSize, maxMessage, (size_t)info.msg_qbytes);
        return -1;
    }
    return 0;
}

static int sendMessage(int id, const struct Message* msg){
    assert(msg);

    int result = 0;
    do {
        result = msgsnd(id, msg, HEADER_SIZE + msg->size, 0);
    } while(result == -1 && errno == EINTR);

    if(result == -1) perror("msgsnd");
    return result;
}

static int receiveMessage(int id, struct Message* msg, size_t chunkSize){
    assert(msg);

    ssize_t received = 0;
    do {
        received = msgrcv(id, msg, HEADER_SIZE + chunkSize, MESSAGE_TYPE, 0);
    } while(received == -1 && errno == EINTR);

    if(received == -1){
        perror("msgrcv (check receiver chunk size)");
        return -1;
    }

    if((size_t)received < HEADER_SIZE || msg->size > chunkSize ||
       (size_t)received != HEADER_SIZE + msg->size ||
       (msg->isLast != 0 && msg->isLast != 1) || (msg->isLast && msg->size != 0)){
        fprintf(stderr, "Invalid queue message\n");
        return -1;
    }
    return 0;
}

static int sendBuffer(int id, struct Message* msg, const FileBuffer* buffer,
                      size_t chunkSize){
    assert(msg);
    assert(buffer);

    msg->mtype = MESSAGE_TYPE;
    size_t sent = 0;
    while(sent < buffer->size){
        size_t remaining = buffer->size - sent;
        msg->size = remaining > chunkSize ? chunkSize : remaining;
        memcpy(msg->data, buffer->data + sent, msg->size);

        if(sendMessage(id, msg) == -1) return -1;
        sent += msg->size;
    }

    msg->size = 0;
    msg->isLast = 1;
    return sendMessage(id, msg);
}

static int receiveBuffer(int id, struct Message* msg, FileBuffer* buffer,
                         size_t chunkSize){
    assert(msg);
    assert(buffer);

    size_t readSize = 0;
    while(1){
        if(receiveMessage(id, msg, chunkSize) == -1) return -1;
        if(msg->isLast) break;

        if(msg->size > SIZE_MAX - readSize){
            fprintf(stderr, "Queue file size overflow\n");
            return -1;
        }

        size_t needed = readSize + msg->size;
        if(needed > buffer->size && reallocFileBuffer(buffer, needed) == -1) return -1;
        if(msg->size > 0) memcpy(buffer->data + readSize, msg->data, msg->size);
        readSize = needed;
    }

    buffer->size = readSize;
    return 0;
}

int queueSend(key_t key, FileBuffer* buffer, size_t chunkSize){
    assert(buffer);

    struct Message* msg = createMessage(chunkSize);
    if(msg == NULL) return -1;

    int id = getQueue(key, IPC_CREAT | IPC_EXCL | 0666);
    if(id == -1){
        free(msg);
        return -1;
    }

    int result = checkQueueCapacity(id, chunkSize);
    if(result == 0) result = sendBuffer(id, msg, buffer, chunkSize);
    if(result == -1) removeQueue(id);

    free(msg);
    return result;
}

int queueRead(key_t key, FileBuffer* buffer, size_t chunkSize){
    assert(buffer);

    struct Message* msg = createMessage(chunkSize);
    if(msg == NULL) return -1;

    int id = getQueue(key, 0);
    if(id == -1){
        free(msg);
        return -1;
    }

    int result = receiveBuffer(id, msg, buffer, chunkSize);
    if(removeQueue(id) == -1) result = -1;
    free(msg);
    return result;
}
