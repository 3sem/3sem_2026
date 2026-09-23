#include <assert.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "transfer.h"
#include "fileBuffer.h"

#include "ipc/shMem.h"

key_t shMemKey = ftok("/tmp/myShm", 'S');

void send(const char* inputFileName){
    assert(inputFileName);

    FileBuffer buffer = {};
    readFileBuffer(inputFileName, &buffer);

    freeFileBuffer(&buffer);
}

void receive(const char* outputFileName){
    assert(outputFileName);

    FileBuffer buffer = {0};
    
    writeFileBuffer(outputFileName, &buffer);

    freeFileBuffer(&buffer);
}
