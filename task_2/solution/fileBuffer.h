#ifndef FILE_BUFFER_H
#define FILE_BUFFER_H

#include <stddef.h>

struct FileBuffer
{
    char* data;
    size_t size;
};

int readFileBuffer(const char* fileName, FileBuffer* buffer);
void freeFileBuffer(FileBuffer* buffer);

#endif /* FILE_BUFFER_H */
