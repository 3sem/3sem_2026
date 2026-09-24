#include <assert.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "transfer.h"
#include "fileBuffer.h"

#include "ipc/shMem.h"

void send(const char* inputFileName){
    assert(inputFileName);

    FileBuffer buffer = {};
    readFileBuffer(inputFileName, &buffer);
    
    key_t shMemKey = ftok("/tmp/myShm", 'S');
    shMemSend(shMemKey, &buffer);

    freeFileBuffer(&buffer);
}

void receive(const char* outputFileName){
    assert(outputFileName);

    FileBuffer buffer = {0};
    
    key_t shMemKey = ftok("/tmp/myShm", 'S');
    shMemRead(shMemKey, &buffer);

    writeFileBuffer(outputFileName, &buffer);

    freeFileBuffer(&buffer);
}
