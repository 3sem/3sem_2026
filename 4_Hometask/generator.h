#ifndef GENERATOR_H
#define GENERATOR_H

#include <stddef.h>
#include <pthread.h>
#include <sys/types.h>

typedef struct threadArgs_t {
    size_t numOfThreads;
    double xOffset;
    int* sumPointsBelowGraph;
    pthread_mutex_t* mutex;
} threadArgs_t;

double integrableFunc(double x);

void* generatePoints(void* args);

ssize_t getNumOfThreads(int argc, char* argv[]);

int* openSharedCounter(void);

size_t createThreads(pthread_t* tidArr, threadArgs_t* threadArgs,
                     size_t numOfThreads, int* sumPointsBelowGraph,
                     pthread_mutex_t* mutex);

int joinThreads(pthread_t* tidArr, size_t numOfThreads);

void cleanupGenerator(threadArgs_t* threadArgs, pthread_t* tidArr,
                      pthread_mutex_t* mutex, int* sumPointsBelowGraph);

#endif
