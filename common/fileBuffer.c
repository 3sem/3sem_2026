#include "fileBuffer.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int readFileBuffer(const char* fileName, FileBuffer* buffer)
{
    assert( fileName);
    assert( buffer);
    assert( !buffer->data && buffer->size == 0);

    FILE* file = fopen( fileName, "rb");
    assert(file);

    struct stat fileInfo = {0};
    if(fstat(fileno(file), &fileInfo) != 0){
        perror("file size");
        fclose(file);
        return -1;
    }

    buffer->size = (size_t) fileInfo.st_size;

    buffer->data = (char*) malloc(buffer->size ? buffer->size : 1);
    assert(buffer->data);

    if( fread( buffer->data, 1, buffer->size, file) != buffer->size )
    {
        printf("File not read!\n");
        freeFileBuffer(buffer);
        fclose(file);
        return -1;
    }

    fclose(file);
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

    // printf("newSize = %lu\n", newSize);
    char* newData = realloc(buffer->data, newSize);
    if (!newData) {
        perror("buffer realloc");
        return -1;
    }

    buffer->data = newData;
    buffer->size = newSize;
    return 0;
}

void freeFileBuffer(FileBuffer* buffer)
{
    assert(buffer);

    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}
