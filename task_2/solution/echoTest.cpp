#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "echoTest.h"
#include "fullDuplexPipe.h"
#include "fileBuffer.h"

struct SendFileArgs{
    fDupPipe_t* fDupPipe;
    const FileBuffer* buffer;
};

const size_t FILE_CHUNK_SIZE = 4096;

void* sendFile(void* args){
    SendFileArgs* sendArgs = (SendFileArgs*) args;
    fDupPipe_t* fDupPipe = sendArgs->fDupPipe;
    const FileBuffer* buffer = sendArgs->buffer;

    size_t sent = 0;

    while(sent < buffer->size){
        size_t count = buffer->size - sent;
        if(count > FILE_CHUNK_SIZE) count = FILE_CHUNK_SIZE;

        ssize_t written = fDupPipe->operations.writeDown(
            fDupPipe, buffer->data + sent, count);
        if(written < 0) continue;
        if(written <= 0){ perror("writeDown"); exit(1); }

        sent += (size_t) written;
    }

    return NULL;
}

void echoFile(fDupPipe_t* fDupPipe){
    char chunk[FILE_CHUNK_SIZE] = {};

    while(true){
        ssize_t count = fDupPipe->operations.readDown(fDupPipe, chunk, sizeof(chunk));
        if(count <= 0) break;
        if(count < 0){ perror("readDown"); exit(1); }

        size_t sent = 0;
        while(sent < (size_t) count){
            ssize_t written = fDupPipe->operations.writeUp(
                fDupPipe, chunk + sent, (size_t) count - sent);
            if(written < 0) continue;
            if(written <= 0){ perror("writeUp"); exit(1); }

            sent += (size_t) written;
        }
    }
}

int runEchoTest(const char* inputFileName, const char* outputFileName){
    fDupPipe_t* fDupPipe = fDupPipeCtor();
    assert(fDupPipe);

    pid_t pid = fDupPipeFork(fDupPipe);

    if(pid > 0){
        FileBuffer input = {};
        if(readFileBuffer(inputFileName, &input) != 0){
            fDupPipeDtor(fDupPipe);
            return 1;
        }

        FileBuffer output = {};
        output.size = input.size;
        output.data = (char*) malloc(output.size);
        assert(output.data);

        struct timespec start = {}, finish = {};
        clock_gettime(CLOCK_MONOTONIC, &start);

        pthread_t senderThId;
        SendFileArgs sendArgs = {fDupPipe, &input};
        int threadError = pthread_create(&senderThId, NULL, sendFile, &sendArgs);
        if (threadError != 0) {
            errno = threadError;
            perror("Ошибка создания потока");
            freeFileBuffer(&input);
            freeFileBuffer(&output);
            fDupPipeDtor(fDupPipe);
            return 1;
        }

        size_t received = 0;
        while(received < output.size){
            ssize_t count = fDupPipe->operations.readUp(
                fDupPipe, output.data + received, output.size - received);
            if(count < 0) continue;

            received += (size_t) count;
        }

        pthread_join(senderThId, NULL);
        clock_gettime(CLOCK_MONOTONIC, &finish);

        fDupPipeDtor(fDupPipe);
        fDupPipe = NULL;

        double seconds = (finish.tv_sec - start.tv_sec) +
                         (finish.tv_nsec - start.tv_nsec) / 1e9;
        printf("Exchange time: %.6f s\n", seconds);

        FILE* file = fopen(outputFileName, "wb");
        assert(file);

        fwrite(output.data, 1, output.size, file);
        fclose(file);

        freeFileBuffer(&input);
        freeFileBuffer(&output);
    }
    else if(pid == 0){
        echoFile(fDupPipe);
        fDupPipeDtor(fDupPipe);
        _exit(0);
    }

    if(fDupPipe) fDupPipeDtor(fDupPipe);
    return 0;
}
