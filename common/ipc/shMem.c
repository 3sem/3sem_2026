#include "shMem.h"

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

void shMemSend(key_t key, FileBuffer* buffer){
    assert(buffer);

    int shmid = shmget(key, 4096, IPC_CREAT | IPC_EXCL | 0666);

    void *shmPtr = shmat(shmid, NULL, 0);
    if (shmPtr == (void *)-1) {
        perror("shmat");
    }

    printf("A reader is connected\n");

    int  sendSize = 0;
    char buf[BUFFER_SIZE] = "";
    char* curPtr = buffer->data;

    while(sendSize < buffer->size){
        int curSize = (buffer->size - sendSize) > BUFFER_SIZE ? BUFFER_SIZE : (buffer->size - sendSize);

        memcpy(buf, curPtr, curSize);
            
        curPtr += curSize;
        sendSize += curSize; 

        memcpy(shmPtr, buf, curSize);
    }
    
    shmdt(shmPtr)ж
}

void shMemRead(key_t key, FileBuffer* buffer){
    assert(buffer);

    if(buffer->size == 0){
        buffer->data = calloc(BUFFER_SIZE, sizeof(char));
        buffer->size = BUFFER_SIZE;
    } 

    int shmid = shmget(key, 4096, 0);    
    if (shmid == -1) {
        perror("shmget");
    }

    shmat(shmid, NULL, SHM_RDONLY);
    if (ptr == (void *)-1) {
        perror("shmat");
    }

    printf("A writer is connected\n");

    int  readSize = 0;
    size_t needed = 0;
    char buf[BUFFER_SIZE] = "";

    int  curSize = 0;
    while(( curSize = read(fd, buf, sizeof(buf)-1)) > 0){
        needed = readSize + (size_t)curSize;
        if(needed > buffer->size){
            reallocFileBuffer(buffer, buffer->size * 2);
        }

        memcpy(buffer->data + readSize, buf, curSize);
            
        readSize += curSize; 
    }
}