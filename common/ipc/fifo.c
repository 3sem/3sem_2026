#include "fifo.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int closeFifo(int fd){
    int result = close(fd);
    if(result == -1) perror("close FIFO");
    return result;
}

int fifoSend(const char* fifoName, FileBuffer* buffer, size_t chunkSize){
    assert(fifoName);
    assert(buffer);
    if(chunkSize == 0) return -1;

    int fd = openFifo(fifoName, O_WRONLY);
    if(fd == -1) return -1;

    int result = 0;
    size_t sent = 0;
    while(sent < buffer->size){
        size_t remaining = buffer->size - sent;
        size_t size = remaining > chunkSize ? chunkSize : remaining;

        if(writeAll(fd, buffer->data + sent, size) == -1){
            result = -1;
            break;
        }

        sent += size;
    }

    if(closeFifo(fd) == -1) result = -1;
    return result;
}

int fifoRead(const char* fifoName, FileBuffer* buffer, size_t chunkSize){
    assert(fifoName);
    assert(buffer);
    if(chunkSize == 0) return -1;

    char* data = calloc(chunkSize, 1);
    if(data == NULL){
        perror("FIFO buffer allocation");
        return -1;
    }

    int fd = openFifo(fifoName, O_RDONLY);
    if(fd == -1){
        free(data);
        return -1;
    }

    int result = 0;
    size_t readSize = 0;

    ssize_t n = 0;
    while((n = readChunk(fd, data, chunkSize)) > 0){
        if((size_t)n > SIZE_MAX - readSize){
            fprintf(stderr, "FIFO file size overflow\n");
            result = -1;
            break;
        }

        size_t needed = readSize + (size_t)n;
        if(needed > buffer->size && reallocFileBuffer(buffer, needed) == -1){
            result = -1;
            break;
        }

        memcpy(buffer->data + readSize, data, (size_t)n);
        readSize = needed;
    }

    buffer->size = readSize;
    if(n == -1) result = -1;
    if(closeFifo(fd) == -1) result = -1;
    free(data);
    return result;
}
