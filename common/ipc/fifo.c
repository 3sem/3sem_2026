#include "fifo.h"

#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define BUFFER_SIZE 4096

void fifoSend(const char* fifoName, FileBuffer* buffer){
    assert(fifoName);

    mknod(fifoName, S_IFIFO | 0666, 0);
    printf("Waiting for a reader\n");
    int fd = open(fifoName, O_WRONLY);
    printf("A reader is connected\n");

    int  sendSize = 0;
    char buf[BUFFER_SIZE] = "";
    char* curPtr = buffer->data;

    while(sendSize < buffer->size){
        int curSize = (buffer->size - sendSize) > BUFFER_SIZE ? BUFFER_SIZE : (buffer->size - sendSize);

        memcpy(buf, curPtr, curSize);
            
        curPtr += curSize;
        sendSize += curSize; 

        write(fd, buf, curSize);

    }
    close(fd);
}

void fifoRead(const char* fifoName, FileBuffer* buffer){
    assert(fifoName);

    if(buffer->size == 0){
        buffer->data = calloc(BUFFER_SIZE, sizeof(char));
        buffer->size = BUFFER_SIZE;
    } 

    if(mknod(fifoName, S_IFIFO | 0666, 0) == -1){
        perror("mkfifo");
        return;
    }
    
    fprintf(stderr, "Before open: %s\n", fifoName);
    
    int fd = open(fifoName, O_RDONLY);
    if (fd == -1) {
        perror("open fifo");
    }

    fprintf(stderr, "After open: fd=%d\n", fd);

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