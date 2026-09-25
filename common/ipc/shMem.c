#include "shMem.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <errno.h>
#include <stdint.h>

#include <semaphore.h>

#define CHUNK_SIZE 4096

struct SharedMessage {
    sem_t empty;
    sem_t full;

    size_t size;
    int finished;
    char data[CHUNK_SIZE];
};

static int getSharedMemory(key_t key, int flags){
    int id = shmget(key, sizeof(struct SharedMessage), flags);
    if(id == -1) perror("shmget");

    return id;
}

static struct SharedMessage* attachSharedMemory(int id){
    void* memory = shmat(id, NULL, 0);
    if(memory == (void*)-1){
        perror("shmat");
        return NULL;
    }

    return memory;
}

static int waitSemaphore(sem_t* semaphore){
    assert(semaphore);

    int result = 0;
    do {
        result = sem_wait(semaphore);
    } while(result == -1 && errno == EINTR);

    if(result == -1) perror("sem_wait");

    return result;
}

static int postSemaphore(sem_t* semaphore){
    assert(semaphore);

    int result = sem_post(semaphore);
    if(result == -1) perror("sem_post");

    return result;
}

static int initSemaphores(struct SharedMessage* msg){
    assert(msg);

    if(sem_init(&msg->empty, 1, 1) == -1){
        perror("sem_init empty");
        return -1;
    }

    if(sem_init(&msg->full, 1, 0) == -1){
        perror("sem_init full");
        if(sem_destroy(&msg->empty) == -1) perror("sem_destroy empty");
        return -1;
    }

    return 0;
}

static void destroySemaphores(struct SharedMessage* msg){
    if(sem_destroy(&msg->empty) == -1) perror("sem_destroy empty");
    if(sem_destroy(&msg->full) == -1) perror("sem_destroy full");
}

static void detachSharedMemory(struct SharedMessage* msg){
    if(shmdt(msg) == -1) perror("shmdt");
}

static void removeSharedMemory(int id){
    if(shmctl(id, IPC_RMID, NULL) == -1) perror("shmctl");
}

static int sendBuffer(struct SharedMessage* msg, const FileBuffer* buffer){
    assert(msg);
    assert(buffer);

    size_t sendSize = 0;
    while(sendSize < buffer->size){
        size_t remaining = buffer->size - sendSize;
        size_t curSize = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

        if(waitSemaphore(&msg->empty) == -1) return -1;

        msg->size = curSize;
        msg->finished = 0;
        memcpy(msg->data, buffer->data + sendSize, curSize);

        if(postSemaphore(&msg->full) == -1) return -1;

        sendSize += curSize;
    }

    if(waitSemaphore(&msg->empty) == -1) return -1;

    msg->size = 0;
    msg->finished = 1;

    if(postSemaphore(&msg->full) == -1) return -1;

    return waitSemaphore(&msg->empty);
}

static int receiveBuffer(struct SharedMessage* msg, FileBuffer* buffer){
    assert(msg);
    assert(buffer);

    size_t readSize = 0;
    size_t needed = 0;

    size_t curSize = 0;
    while(1){
        if(waitSemaphore(&msg->full) == -1) return -1;

        if(msg->finished){
            if(postSemaphore(&msg->empty) == -1) return -1;
            break;
        }

        curSize = msg->size;
        if(curSize > sizeof(msg->data) || curSize > SIZE_MAX - readSize){
            fprintf(stderr, "Invalid shared memory message size\n");
            return -1;
        }

        needed = readSize + (size_t)curSize;
        if(needed > buffer->size){
            if(reallocFileBuffer(buffer, needed) == -1) return -1;
        }

        memcpy(buffer->data + readSize, msg->data, curSize);

        if(postSemaphore(&msg->empty) == -1) return -1;

        readSize += curSize;
    }

    buffer->size = readSize;

    return 0;
}

void shMemSend(key_t key, FileBuffer* buffer){
    assert(buffer);

    int shmid = getSharedMemory(key, IPC_CREAT | IPC_EXCL | 0666);
    if(shmid == -1) return;

    struct SharedMessage* msg = attachSharedMemory(shmid);
    if(msg == NULL){
        removeSharedMemory(shmid);
        return;
    }

    if(initSemaphores(msg) == 0 && sendBuffer(msg, buffer) == 0){
        destroySemaphores(msg);
    }

    detachSharedMemory(msg);
    removeSharedMemory(shmid);
}

void shMemRead(key_t key, FileBuffer* buffer){
    assert(buffer);

    if(buffer->size == 0){
        if(reallocFileBuffer(buffer, CHUNK_SIZE) == -1) return;
    }

    int shmid = getSharedMemory(key, 0);
    if(shmid == -1) return;

    struct SharedMessage* msg = attachSharedMemory(shmid);
    if(msg == NULL) return;

    receiveBuffer(msg, buffer);
    detachSharedMemory(msg);
}
