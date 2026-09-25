#include <assert.h>
#include <stdio.h>

#include "transfer.h"
#include "fileBuffer.h"
#include "ipc/shMem.h"

int send(const char* inputFileName, size_t chunkSize){
    assert(inputFileName);

    FileBuffer buffer = {0};
    if(readFileBuffer(inputFileName, &buffer) == -1) return -1;

    key_t key = ftok(SHMEM_PATH, 'S');
    if(key == (key_t)-1){
        perror("ftok");
        freeFileBuffer(&buffer);
        return -1;
    }

    int result = shMemSend(key, &buffer, chunkSize);
    freeFileBuffer(&buffer);
    return result;
}

int receive(const char* outputFileName, size_t chunkSize){
    assert(outputFileName);

    FileBuffer buffer = {0};
    key_t key = ftok(SHMEM_PATH, 'S');
    if(key == (key_t)-1){
        perror("ftok");
        return -1;
    }

    int result = shMemRead(key, &buffer, chunkSize);
    if(result == 0){
        result = writeFileBuffer(outputFileName, &buffer);
    }

    freeFileBuffer(&buffer);
    return result;
}
