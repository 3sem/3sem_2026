#include "queue.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>

// #include <semaphore.h>

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

void queueSend(key_t key, FileBuffer* buffer){
    assert(buffer);

    int msqid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    if (msqid == -1) {
        perror("msgget");
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

        if (msgsnd(msqid, &msg, sizeof(msg.payload), 0) == -1) {
            perror("msgsnd");
            return;
        }

        sendSize += curSize; 
    }

    msg.payload.size = 0;
    msg.payload.isLast = 1;
    msgsnd(msqid, &msg, sizeof(struct Payload), 0);
}

void queueRead(key_t key, FileBuffer* buffer){
    assert(buffer);

    if(buffer->size == 0){
        buffer->data = calloc(MESSAGE_SIZE, sizeof(char));
        buffer->size = MESSAGE_SIZE;
    } 

    int msqid = msgget(key, 0);    
    if (msqid == -1) {
        perror("msgget");
    }

    struct Message msg = {};

    size_t  readSize = 0;
    size_t needed = 0;

    size_t curSize = 0;
    while(1){
        ssize_t received = msgrcv(msqid, &msg, sizeof(msg.payload), MESSAGE_TYPE, 0);
        if (received == -1) {
            perror("msgrcv");
            return;
        }

        if ((size_t)received != sizeof(msg.payload) ||
            msg.payload.size > sizeof(msg.payload.data)) {
            fprintf(stderr, "Invalid message\n");
            return;
        }

        if(msg.payload.isLast){
            break;
        }

        curSize = msg.payload.size;
        
        needed = readSize + (size_t)curSize;
        if(needed > buffer->size){
            reallocFileBuffer(buffer, buffer->size * 2);
        }

        memcpy(buffer->data + readSize, msg.payload.data, msg.payload.size);
            
        readSize += curSize; 
    }

    buffer->size = readSize;

    if(msgctl(msqid, IPC_RMID, NULL) == -1){
        perror("msgctl");
    }
}