#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct FileBuffer
{
    char* data;
    size_t size;
    bool isMapped;
} FileBuffer;

int readFileBuffer(const char* fileName, FileBuffer* buffer);
int writeFileBuffer(const char* fileName, const FileBuffer* buffer);

int reallocFileBuffer(FileBuffer* buffer, size_t newSize);
void freeFileBuffer(FileBuffer* buffer);

#endif /* FILE_BUFFER_H */
