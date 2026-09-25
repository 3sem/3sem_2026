#include "fifo.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static int openFifo(const char* path, int flags){
    if(mkfifo(path, 0666) == -1){
        if(errno != EEXIST){
            perror("mkfifo");
            return -1;
        }

        struct stat info = {0};
        if(lstat(path, &info) == -1){
            perror("lstat");
            return -1;
        }

        if(!S_ISFIFO(info.st_mode)){
            fprintf(stderr, "Not a FIFO: %s\n", path);
            return -1;
        }
    }

    int fd = -1;
    do {
        fd = open(path, flags);
    } while(fd == -1 && errno == EINTR);

    if(fd == -1) perror("open FIFO");

    return fd;
}

static int writeAll(int fd, const char* data, size_t size){
    size_t written = 0;
    while(written < size){
        ssize_t n = write(fd, data + written, size - written);
        if(n == -1 && errno == EINTR) continue;

        if(n <= 0){
            if(n == 0) errno = EIO;

            perror("write FIFO");
            return -1;
        }

        written += (size_t)n;
    }

    return 0;
}

static ssize_t readChunk(int fd, char* data, size_t size){
    ssize_t n = 0;
    do {
        n = read(fd, data, size);
    } while(n == -1 && errno == EINTR);

    if(n == -1) perror("read FIFO");

    return n;
}

static void closeFifo(int fd){
    if(close(fd) == -1) perror("close FIFO");
}

void fifoSend(const char* fifoName, FileBuffer* buffer){
    assert(fifoName);
    assert(buffer);

    int fd = openFifo(fifoName, O_WRONLY);
    if(fd == -1) return;

    size_t sent = 0;
    while(sent < buffer->size){
        size_t remaining = buffer->size - sent;
        size_t size = remaining > BUFFER_SIZE ? BUFFER_SIZE : remaining;

        if(writeAll(fd, buffer->data + sent, size) == -1) break;

        sent += size;
    }

    closeFifo(fd);
}

void fifoRead(const char* fifoName, FileBuffer* buffer){
    assert(fifoName);
    assert(buffer);

    int fd = openFifo(fifoName, O_RDONLY);
    if(fd == -1) return;

    size_t readSize = 0;
    char data[BUFFER_SIZE] = {0};

    ssize_t n = 0;
    while((n = readChunk(fd, data, sizeof(data))) > 0){
        if((size_t)n > SIZE_MAX - readSize){
            fprintf(stderr, "FIFO file size overflow\n");
            break;
        }

        size_t needed = readSize + (size_t)n;
        if(needed > buffer->size && reallocFileBuffer(buffer, needed) == -1) break;

        memcpy(buffer->data + readSize, data, (size_t)n);
        readSize = needed;
    }

    buffer->size = readSize;
    closeFifo(fd);
}
