#include <assert.h>

#include "transfer.h"

#include "ipc/fifo.h"

#include "fileBuffer.h"

void send(const char* fifoName, const char* inputFileName){
    assert(fifoName);
    assert(inputFileName);

    FileBuffer buffer = {};
    readFileBuffer(inputFileName, &buffer);
    
    fifoSend(fifoName, &buffer);

    freeFileBuffer(&buffer);
}

void receive(const char* fifoName, const char* outputFileName){
    assert(fifoName);
    assert(outputFileName);

    FileBuffer buffer = {0};
    fifoRead(fifoName, &buffer);
    
    writeFileBuffer(outputFileName, &buffer);

    freeFileBuffer(&buffer);
}
