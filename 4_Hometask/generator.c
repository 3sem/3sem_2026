#define _POSIX_C_SOURCE 200809L

#include "generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char* argv[]) {
    ssize_t parsedNumOfThreads = getNumOfThreads(argc, argv);
    if (parsedNumOfThreads <= 0) {
        return EXIT_FAILURE;
    }
    size_t numOfThreads = (size_t)parsedNumOfThreads;

    int* sumPointsBelowGraph = openSharedCounter();
    if (sumPointsBelowGraph == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    int exitStatus = EXIT_FAILURE;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t* tidArr = calloc(numOfThreads, sizeof(*tidArr));
    threadArgs_t* threadArgs = calloc(numOfThreads, sizeof(*threadArgs));

    if (!tidArr || !threadArgs) {
        fprintf(stderr, "ERROR! Cannot allocate thread arrays\n");
        goto cleanup;
    }

    size_t created = createThreads(tidArr, threadArgs, numOfThreads,
                                    sumPointsBelowGraph, &mutex);

    if (joinThreads(tidArr, created) != 0) {
        exit(EXIT_FAILURE);
    }

    if (created == numOfThreads) {
        exitStatus = EXIT_SUCCESS;
    }

cleanup:
    cleanupGenerator(threadArgs, tidArr, &mutex, sumPointsBelowGraph);
    return exitStatus;
}
