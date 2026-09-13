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

    struct stat fileInfo = {};
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

void freeFileBuffer(FileBuffer* buffer)
{
    assert(buffer);

    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}
