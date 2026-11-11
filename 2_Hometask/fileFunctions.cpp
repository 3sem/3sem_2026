#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "fileFunctions.h"

void closeFd(int* fd) {
    assert(fd);

    if (*fd == INIT_FD) {
        return;
    }

    if (close(*fd) == -1) {
        perror("Failed close.");
    }

    *fd = INIT_FD;
}

ssize_t readFull(int fd, char* buf, size_t size) {
    assert(buf);

    size_t total = 0;

    while (total < size) {
        ssize_t received = read(fd, buf + total, size - total);

        if (received == -1 && errno == EINTR)
            continue;

        if (received == -1) {
            perror("Failed read.");
            return -1;
        }

        if (received == 0)
            break;

        total += (size_t)received;
    }

    return (ssize_t)total;
}

int writeFull(int fd, const char* buf, size_t size) {
    assert(buf);

    size_t total = 0;

    while (total < size) {
        ssize_t written = write(fd, buf + total, size - total);

        if (written == -1 && errno == EINTR)
            continue;

        if (written <= 0) {
            perror("Failed write.");
            return -1;
        }

        total += (size_t)written;
    }

    return 0;
}
