#include <assert.h>
#include <stdio.h>

#include "transfer.h"
#include "fileBuffer.h"
#include "ipc/queue.h"

int send(const char* inputFileName, size_t chunkSize){
    assert(inputFileName);

    FileBuffer buffer = {0};
    if(readFileBuffer(inputFileName, &buffer) == -1) return -1;

    key_t key = ftok(QUEUE_PATH, 'S');
    if(key == (key_t)-1){
        perror("ftok");
        freeFileBuffer(&buffer);
        return -1;
    }

    int result = queueSend(key, &buffer, chunkSize);
    freeFileBuffer(&buffer);
    return result;
}

int receive(const char* outputFileName, size_t chunkSize){
    assert(outputFileName);

    FileBuffer buffer = {0};
    key_t key = ftok(QUEUE_PATH, 'S');
    if(key == (key_t)-1){
        perror("ftok");
        return -1;
    }

    int result = queueRead(key, &buffer, chunkSize);
    if(result == 0){
        result = writeFileBuffer(outputFileName, &buffer);
    }

    freeFileBuffer(&buffer);
    return result;
}
