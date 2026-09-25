#include "queue.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <errno.h>
#include <stdint.h>

#define MESSAGE_SIZE 4096

#define MESSAGE_TYPE 333

struct Payload {
    size_t size;
    int isLast;
    char data[MESSAGE_SIZE];
};

struct Message {
    long mtype;
    struct Payload payload;
};

static int getQueue(key_t key, int flags){
    int id = msgget(key, flags);
    if(id == -1) perror("msgget");

    return id;
}

static int sendMessage(int id, const struct Message* msg){
    assert(msg);

    int result = 0;
    do {
        result = msgsnd(id, msg, sizeof(msg->payload), 0);
    } while(result == -1 && errno == EINTR);

    if(result == -1) perror("msgsnd");

    return result;
}

static int receiveMessage(int id, struct Message* msg){
    assert(msg);

    ssize_t received = 0;
    do {
        received = msgrcv(id, msg, sizeof(msg->payload), MESSAGE_TYPE, 0);
    } while(received == -1 && errno == EINTR);

    if(received == -1){
        perror("msgrcv");
        return -1;
    }

    if((size_t)received != sizeof(msg->payload) ||
       msg->payload.size > sizeof(msg->payload.data)){
        fprintf(stderr, "Invalid message\n");
        return -1;
    }

    return 0;
}

static void removeQueue(int id){
    if(msgctl(id, IPC_RMID, NULL) == -1) perror("msgctl");
}

void queueSend(key_t key, FileBuffer* buffer){
    assert(buffer);

    int msqid = getQueue(key, IPC_CREAT | IPC_EXCL | 0666);
    if (msqid == -1) {
        return;
    }

    struct Message msg = {
        .mtype = MESSAGE_TYPE,
        .payload.isLast = 0
    };

    size_t sendSize = 0;
    while(sendSize < buffer->size){
        size_t remaining = buffer->size - sendSize;
        size_t curSize = remaining > MESSAGE_SIZE ? MESSAGE_SIZE : remaining;

        msg.payload.size = curSize;

        memcpy(msg.payload.data, buffer->data + sendSize, curSize);

        if (sendMessage(msqid, &msg) == -1) {
            return;
        }

        sendSize += curSize;
    }

    msg.payload.size = 0;
    msg.payload.isLast = 1;
    if(sendMessage(msqid, &msg) == -1) return;
}

void queueRead(key_t key, FileBuffer* buffer){
    assert(buffer);

    if(buffer->size == 0){
        if(reallocFileBuffer(buffer, MESSAGE_SIZE) == -1) return;
    }

    int msqid = getQueue(key, 0);
    if (msqid == -1) {
        return;
    }

    struct Message msg = {};

    size_t  readSize = 0;
    size_t needed = 0;

    size_t curSize = 0;
    while(1){
        if(receiveMessage(msqid, &msg) == -1) return;

        if(msg.payload.isLast){
            break;
        }

        curSize = msg.payload.size;
        if(curSize > SIZE_MAX - readSize){
            fprintf(stderr, "Queue file size overflow\n");
            return;
        }

        needed = readSize + (size_t)curSize;
        if(needed > buffer->size){
            if(reallocFileBuffer(buffer, needed) == -1) return;
        }

        memcpy(buffer->data + readSize, msg.payload.data, msg.payload.size);

        readSize += curSize;
    }

    buffer->size = readSize;

    removeQueue(msqid);
}
