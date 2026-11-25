#include "fifo.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int prepareFifo(const char *path) {
    if (mkfifo(path, 0600) == 0) {
        return 0;
    }

    if (errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    struct stat fifo_info;
    if (lstat(path, &fifo_info) == -1) {
        perror("lstat FIFO");
        return -1;
    }

    if (!S_ISFIFO(fifo_info.st_mode)) {
        fprintf(stderr, "Path exists but is not a FIFO: %s\n", path);
        return -1;
    }

    return 0;
}

static int openFifo(const char *path, int flags) {
    int fd;

    do {
        fd = open(path, flags);
    } while (fd == -1 && errno == EINTR);

    if (fd == -1) {
        perror("open FIFO");
    }

    return fd;
}

int openFifoForReading(const char *path) {
    return openFifo(path, O_RDONLY);
}

int openFifoForWriting(const char *path) {
    return openFifo(path, O_WRONLY);
}

int writeAll(int fd, const void *buffer, size_t size) {
    const char *data = buffer;
    size_t total_written = 0;

    while (total_written < size) {
        ssize_t bytes_written = write(
            fd,
            data + total_written,
            size - total_written
        );

        if (bytes_written == -1 && errno == EINTR) {
            continue;
        }

        if (bytes_written <= 0) {
            if (bytes_written == 0) {
                errno = EIO;
            }

            perror("write FIFO");
            return -1;
        }

        total_written += (size_t)bytes_written;
    }

    return 0;
}

int removeFifo(const char *path) {
    if (unlink(path) == -1 && errno != ENOENT) {
        perror("unlink FIFO");
        return -1;
    }

    return 0;
}
