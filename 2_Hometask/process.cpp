#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "process.h"
#include "fileFunctions.h"

process_t* processCtor () {
    process_t* process = (process_t*)calloc(1, sizeof(process_t));
    if (!process) {
        perror("Failed process calloc.");
        return NULL;
    }

    process->fdIn = INIT_FD;
    process->fdOut = INIT_FD;

    return process;
}

void processDtor (process_t* process) {
    if (!process) {
        return;
    }

    closeFd(&process->fdIn);
    closeFd(&process->fdOut);

    free(process);
}

int connectProcesses(process_t *first, process_t *second) {
    assert(first);
    assert(second);

    int firstToSecond[2] = {INIT_FD, INIT_FD};
    int secondToFirst[2] = {INIT_FD, INIT_FD};

    if (pipe(firstToSecond) == -1) {
        perror("pipe firstToSecond");
        return -1;
    }

    if (pipe(secondToFirst) == -1) {
        perror("pipe secondToFirst");

        close(firstToSecond[0]);
        close(firstToSecond[1]);

        return -1;
    }

    first->fdOut = firstToSecond[1];
    second->fdIn = firstToSecond[0];

    second->fdOut = secondToFirst[1];
    first->fdIn = secondToFirst[0];

    return 0;
}

ssize_t processRead(process_t* process, char* buf, size_t size) {
    assert(process);

    return readFull(process->fdIn, buf, size);
}

int processWrite(process_t* process, const char* buf, size_t size) {
    assert(process);

    return writeFull(process->fdOut, buf, size);
}

void processFinishWrite(process_t* process) {
    assert(process);

    closeFd(&process->fdOut);
}

int processExchange(process_t* process, char* buf, size_t size, int lastBlock) {
    assert(process);
    assert(buf);

    if (processWrite(process, buf, size) == -1) {
        return -1;
    }

    if (lastBlock) {
        processFinishWrite(process);
    }

    ssize_t received = processRead(process, buf, size);

    if (received == -1)
        return -1;

    if ((size_t)received != size) {
        fprintf(stderr, "Error: incomplete echo.\n");
        return -1;
    }

    return 0;
}
