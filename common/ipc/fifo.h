#ifndef FIFO_H 
#define FIFO_H

#include "fileBuffer.h"

int fifoSend(const char* fifoName, FileBuffer* buffer, size_t chunkSize);
int fifoRead(const char* fifoName, FileBuffer* buffer, size_t chunkSize);

#endif /* FIFO_H */