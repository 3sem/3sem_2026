#include "shMem.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>

#include <semaphore.h>

#define CHUNK_SIZE 4096

struct SharedMessage {
    sem_t empty;          
    sem_t full;           

    size_t size;       
    int finished;      
    char data[CHUNK_SIZE];
};

void shMemSend(key_t key, FileBuffer* buffer){
    assert(buffer);

    int shmid = shmget(key, sizeof(struct SharedMessage), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        perror("shmget");
        return;
    }   

    struct SharedMessage* msg = shmat(shmid, NULL, 0);
    if (msg == (void *)-1) {
        perror("shmat");        
        return;
    }

    sem_init(&msg->empty, 1, 1);
    sem_init(&msg->full, 1, 0);

    size_t sendSize = 0;
    while(sendSize < buffer->size){
        size_t remaining = buffer->size - sendSize;
        size_t curSize = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

        sem_wait(&msg->empty);

        msg->size = curSize;
        msg->finished = 0;
        memcpy(msg->data, buffer->data + sendSize, curSize);
            
        sem_post(&msg->full);

        sendSize += curSize; 
    }
    
    sem_wait(&msg->empty);

    msg->size = 0;
    msg->finished = 1;

    sem_post(&msg->full);


    sem_wait(&msg->empty);
    sem_destroy(&msg->empty);
    sem_destroy(&msg->full);
    shmdt(msg);
    shmctl(shmid, IPC_RMID, NULL);   
}

void shMemRead(key_t key, FileBuffer* buffer){
    assert(buffer);

    if(buffer->size == 0){
        buffer->data = calloc(CHUNK_SIZE, sizeof(char));
        buffer->size = CHUNK_SIZE;
    } 

    int shmid = shmget(key, sizeof(struct SharedMessage), 0);    
    if (shmid == -1) {
        perror("shmget");
    }

    struct SharedMessage* msg = shmat(shmid, NULL, 0);
    if (msg == (void *)-1) {
        perror("shmat");
    }

    size_t  readSize = 0;
    size_t needed = 0;

    size_t curSize = 0;
    while(1){
        sem_wait(&msg->full);

        if(msg->finished){
            sem_post(&msg->empty);
            break;
        }

        curSize = msg->size;
        needed = readSize + (size_t)curSize;
        if(needed > buffer->size){
            reallocFileBuffer(buffer, buffer->size * 2);
        }

        memcpy(buffer->data + readSize, msg->data, curSize);

        sem_post(&msg->empty);
            
        readSize += curSize; 
    }

    buffer->size = readSize;

    shmdt(msg);
}