#include "fileBuffer.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int readFileBuffer(const char* fileName, FileBuffer* buffer)
{
    assert( fileName);
    assert( buffer);
    assert( !buffer->data && buffer->size == 0 && !buffer->isMapped);

    int fd = open(fileName, O_RDONLY);
    if(fd == -1){
        perror("file open");
        return -1;
    }

    struct stat fileInfo = {0};
    if(fstat(fd, &fileInfo) != 0){
        perror("file size");
        close(fd);
        return -1;
    }

    if(fileInfo.st_size < 0 || (uintmax_t)fileInfo.st_size > SIZE_MAX){
        fprintf(stderr, "File is too large\n");
        close(fd);
        return -1;
    }

    size_t fileSize = (size_t)fileInfo.st_size;
    if(fileSize == 0){
        close(fd);
        return 0;
    }

    void* mapping = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    int mmapError = errno;
    close(fd);

    if(mapping == MAP_FAILED){
        errno = mmapError;
        perror("file mmap");
        return -1;
    }

    buffer->data = mapping;
    buffer->size = fileSize;
    buffer->isMapped = true;
    return 0;
}

int writeFileBuffer(const char* fileName, const FileBuffer* buffer)
{
    assert( fileName);
    assert( buffer);
    assert( buffer->data || buffer->size == 0);

    FILE* file = fopen( fileName, "wb");
    if (!file) {
        perror("file open");
        return -1;
    }

    // printf("bufferSize = 0\n");
    if (buffer->size > 0 &&
        fwrite( buffer->data, 1, buffer->size, file) != buffer->size)
    {
        perror("file write");
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {
        perror("file close");
        return -1;
    }

    return 0;
}

int reallocFileBuffer(FileBuffer* buffer, size_t newSize)
{
    assert(buffer);
    assert(buffer->data || buffer->size == 0);

    if (newSize == 0) {
        freeFileBuffer(buffer);
        return 0;
    }

    if (buffer->isMapped) {
        fprintf(stderr, "Cannot resize a memory-mapped file buffer\n");
        return -1;
    }

    size_t allocationSize = newSize;
    if (buffer->size > 0 && buffer->size <= SIZE_MAX / 2) {
        size_t doubledSize = buffer->size * 2;
        if (doubledSize > allocationSize) allocationSize = doubledSize;
    }

    char* newData = realloc(buffer->data, allocationSize);
    if (!newData) {
        perror("buffer realloc");
        return -1;
    }

    buffer->data = newData;
    buffer->size = allocationSize;
    return 0;
}

void freeFileBuffer(FileBuffer* buffer)
{
    assert(buffer);

    if(buffer->isMapped){
        if(munmap(buffer->data, buffer->size) == -1) perror("file munmap");
    } else {
        free(buffer->data);
    }

    buffer->data = NULL;
    buffer->size = 0;
    buffer->isMapped = false;
}
