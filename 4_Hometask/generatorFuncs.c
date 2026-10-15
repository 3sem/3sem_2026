#define _DEFAULT_SOURCE

#include "generator.h"
#include "common.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

double integrableFunc(double x) {
    return sin(x);
}

void* generatePoints(void* args) {
    assert(args);

    threadArgs_t threadArgs = *(threadArgs_t*)args;
    size_t numOfThreads = threadArgs.numOfThreads;
    double xOffset = threadArgs.xOffset;

    struct drand48_data state = {0};
    srand48_r(threadArgs.seed, &state);

    int numOfPoints = NUM_OF_POINTS / numOfThreads;
    int numPointsBelowGraph = 0;

    for (int i = 0; i < numOfPoints; i++) {
        double u, y;
        drand48_r(&state, &u);
        drand48_r(&state, &y);

        double x = u / (double)numOfThreads + xOffset;

        if (y - integrableFunc(x) < 0) {
            numPointsBelowGraph++;
        }
    }

    pthread_mutex_lock(threadArgs.mutex);
    *threadArgs.sumPointsBelowGraph += numPointsBelowGraph;
    pthread_mutex_unlock(threadArgs.mutex);

    return NULL;
}

ssize_t getNumOfThreads(int argc, char* argv[]) {
    assert(argv);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_of_threads>\n", argv[0]);
        return -1;
    }

    char* end = NULL;
    errno = 0;
    unsigned long numOfThreads = strtoul(argv[1], &end, 10);

    if (!isdigit((unsigned char)argv[1][0]) || *end != '\0' ||
        errno != 0 || numOfThreads == 0 ||
        numOfThreads > NUM_OF_POINTS) {
        fprintf(stderr,
                "ERROR! Number of threads must be between 1 and %d\n",
                NUM_OF_POINTS);
        return -1;
    }

    return (ssize_t)numOfThreads;
}

int* openSharedCounter(void) {
    int fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (fd == -1) {
        perror("shm_open");
        return MAP_FAILED;
    }

    int* counter = mmap(NULL, sizeof(*counter),
                        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (counter == MAP_FAILED) {
        perror("mmap");
    }

    close(fd);
    return counter;
}

size_t createThreads(pthread_t* tidArr, threadArgs_t* threadArgs,
                     size_t numOfThreads, int* sumPointsBelowGraph,
                     pthread_mutex_t* mutex) {
    assert(tidArr);
    assert(threadArgs);
    assert(sumPointsBelowGraph);
    assert(mutex);

    size_t created = 0;

    for (; created < numOfThreads; created++) {
        double curThreadXOffset =
            (double)created / (double)numOfThreads;

        threadArgs[created] = (threadArgs_t) {
            .numOfThreads = numOfThreads,
            .xOffset = curThreadXOffset,
            .sumPointsBelowGraph = sumPointsBelowGraph,
            .mutex = mutex,
            .seed = (long)created + 1
        };

        int error = pthread_create(&tidArr[created], NULL,
                                   generatePoints, &threadArgs[created]);

        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            break;
        }
    }

    return created;
}

int joinThreads(pthread_t* tidArr, size_t numOfThreads) {
    assert(tidArr);

    for (size_t curThreadNum = 0;
         curThreadNum < numOfThreads;
         curThreadNum++) {
        int error = pthread_join(tidArr[curThreadNum], NULL);

        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            return -1;
        }
    }

    return 0;
}

void cleanupGenerator(threadArgs_t* threadArgs, pthread_t* tidArr,
                      pthread_mutex_t* mutex, int* sumPointsBelowGraph) {
    free(threadArgs);
    free(tidArr);
    pthread_mutex_destroy(mutex);
    munmap(sumPointsBelowGraph, sizeof(*sumPointsBelowGraph));
}
