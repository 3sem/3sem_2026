#define _POSIX_C_SOURCE 200809L

#include "integrator.h"
#include "common.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int* mapSharedCounter(int fd) {
    int* result = MAP_FAILED;

    if (ftruncate(fd, sizeof(*result)) == -1) {
        perror("ftruncate");
        return MAP_FAILED;
    }

    result = mmap(NULL, sizeof(*result), PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
    if (result == MAP_FAILED) {
        perror("mmap");
        return MAP_FAILED;
    }

    *result = 0;
    return result;
}

int runGenerator(const char* numOfThreads, int fd) {
    pid_t childPid = fork();
    if (childPid == -1) {
        perror("fork");
        return -1;
    }

    if (childPid == 0) {
        close(fd);
        execl("./generator", "./generator", numOfThreads, (char*)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    int status = 0;
    pid_t waited = 0;

    do {
        waited = waitpid(childPid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        perror("waitpid");
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "ERROR! Program A failed\n");
        return -1;
    }

    return 0;
}

void printIntegrationResult(int pointsBelowGraph) {
    double estimate = (double)pointsBelowGraph / NUM_OF_POINTS;
    double exact = 1.0 - cos(1.0);

    printf("Result = %.9f\n", estimate);
    printf("Exact  = %.9f\n", exact);
}

void cleanupIntegrator(int fd, int* result) {
    if (result != MAP_FAILED) {
        munmap(result, sizeof(*result));
    }

    close(fd);
    shm_unlink(SHM_NAME);
}
