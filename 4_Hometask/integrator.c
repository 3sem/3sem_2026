#define _POSIX_C_SOURCE 200809L

#include "integrator.h"
#include "common.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_of_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    int exitStatus = EXIT_FAILURE;
    int* result = mapSharedCounter(fd);

    if (result == MAP_FAILED) {
        goto cleanup;
    }

    if (runGenerator(argv[1], fd) == -1) {
        goto cleanup;
    }

    printIntegrationResult(*result);
    exitStatus = EXIT_SUCCESS;

cleanup:
    cleanupIntegrator(fd, result);
    return exitStatus;
}
