#include "common.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

size_t getBufferSize(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s buffer_bytes < input > output\n", argv[0]);
        return 0;
    }

    char* end = NULL;
    errno = 0;
    unsigned long long bufferSize = strtoull(argv[1], &end, 10);

    if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end ||
        bufferSize == 0 || bufferSize > (unsigned long long)SSIZE_MAX) {
        fprintf(stderr, "Invalid buffer size\n");
        return 0;
    }

    return (size_t)bufferSize;
}

ssize_t readChunk(int fd, void* buffer, size_t bufferSize) {
    assert(buffer);

    ssize_t dataSize;
    do {
        dataSize = read(fd, buffer, bufferSize);
    } while (dataSize == -1 && errno == EINTR);

    if (dataSize == -1)
        perror("read");

    return dataSize;
}

int writeFull(int fd, const void* buffer, size_t dataSize) {
    assert(buffer);

    const unsigned char* data = buffer;
    size_t writtenSize = 0;

    while (writtenSize < dataSize) {
        ssize_t written = write(fd, data + writtenSize, dataSize - writtenSize);
        if (written == -1 && errno == EINTR)
            continue;
        if (written <= 0) {
            if (written == 0)
                errno = EIO;
            perror("write");
            return -1;
        }
        writtenSize += (size_t)written;
    }

    return 0;
}

int copyFile(int inputFd, int outputFd, size_t bufferSize) {
    unsigned char* buffer = malloc(bufferSize);
    if (!buffer) {
        perror("malloc");
        return -1;
    }

    ssize_t dataSize;
    while ((dataSize = readChunk(inputFd, buffer, bufferSize)) > 0) {
        if (writeFull(outputFd, buffer, (size_t)dataSize) == -1) {
            dataSize = -1;
            break;
        }
    }

    free(buffer);
    return dataSize == -1 ? -1 : 0;
}

void closeFd(int* fd) {
    assert(fd);
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

int waitChild(pid_t childPid) {
    int status = 0;
    pid_t result;

    do {
        result = waitpid(childPid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        fprintf(stderr, "Child process failed\n");
        return -1;
    }

    return 0;
}
