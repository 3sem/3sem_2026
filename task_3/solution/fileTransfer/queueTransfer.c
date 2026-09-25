#include <assert.h>

#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "transfer.h"
#include "fileBuffer.h"

#include "ipc/queue.h"

void send(const char* inputFileName){
    assert(inputFileName);

    FileBuffer buffer = {};
    readFileBuffer(inputFileName, &buffer);
    
    key_t queueKey = ftok(QUEUE_PATH, 'S');
    queueSend(queueKey, &buffer);

    freeFileBuffer(&buffer);
}

void receive(const char* outputFileName){
    assert(outputFileName);

    FileBuffer buffer = {0};
    
    key_t queueKey = ftok(QUEUE_PATH, 'S');
    queueRead(queueKey, &buffer);

    writeFileBuffer(outputFileName, &buffer);

    freeFileBuffer(&buffer);
}
