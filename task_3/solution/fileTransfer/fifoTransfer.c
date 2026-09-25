#include <assert.h>

#include "transfer.h"

#include "ipc/fifo.h"

#include "fileBuffer.h"

void send(const char* inputFileName){
    assert(inputFileName);

    FileBuffer buffer = {};
    readFileBuffer(inputFileName, &buffer);
    
    fifoSend(FIFO_PATH, &buffer);

    freeFileBuffer(&buffer);
}

void receive(const char* outputFileName){
    assert(outputFileName);

    FileBuffer buffer = {0};
    fifoRead(FIFO_PATH, &buffer);
    
    writeFileBuffer(outputFileName, &buffer);

    freeFileBuffer(&buffer);
}
