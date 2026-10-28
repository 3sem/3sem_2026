#include "transfer.h"

#include <errno.h>
#include <unistd.h>

static ssize_t readSome(int fd, void *buffer, size_t size) {
    for (;;) {
        if (shouldStop()) {
            return -1;
        }
        ssize_t count = read(fd, buffer, size);
        if (count >= 0 || errno != EINTR) {
            return count;
        }
    }
}

ssize_t readBlock(int fd, void *buffer, size_t size) {
    unsigned char *data = buffer;
    size_t offset = 0;
    while (offset < size) {
        ssize_t count = readSome(fd, data + offset, size - offset);
        if (count < 0) {
            return -1;
        }
        if (count == 0) {
            break;
        }
        offset += (size_t) count;
    }
    return (ssize_t) offset;
}

int writeAll(int fd, const void *buffer, size_t size) {
    const unsigned char *data = buffer;
    size_t offset = 0;
    while (offset < size) {
        if (shouldStop()) {
            return -1;
        }
        ssize_t count = write(fd, data + offset, size - offset);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        offset += (size_t) count;
    }
    return 0;
}
