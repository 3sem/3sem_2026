#include <assert.h>
#include <stdio.h>

#include "transfer.h"
#include "fileBuffer.h"
#include "ipc/fifo.h"

int send(const char* inputFileName, size_t chunkSize){
    assert(inputFileName);

    FileBuffer buffer = {0};
    if(readFileBuffer(inputFileName, &buffer) == -1) return -1;


    int result = fifoSend(FIFO_PATH, &buffer, chunkSize);
    freeFileBuffer(&buffer);
    return result;
}

int receive(const char* outputFileName, size_t chunkSize){
    assert(outputFileName);

    FileBuffer buffer = {0};

    int result = fifoRead(FIFO_PATH, &buffer, chunkSize);
    if(result == 0){
        result = writeFileBuffer(outputFileName, &buffer);
    }

    freeFileBuffer(&buffer);
    return result;
}
