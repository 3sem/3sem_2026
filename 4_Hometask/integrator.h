#ifndef INTEGRATOR_H
#define INTEGRATOR_H

int* mapSharedCounter(int fd);

int runGenerator(const char* numOfThreads, int fd);

void printIntegrationResult(int pointsBelowGraph);

void cleanupIntegrator(int fd, int* result);

#endif
