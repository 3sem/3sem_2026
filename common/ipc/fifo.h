#ifndef FIFO_H 
#define FIFO_H

#include "fileBuffer.h"

void fifoSend(const char* fifoName, FileBuffer* buffer);
void fifoRead(const char* fifoName, FileBuffer* buffer);

#endif /* FIFO_H */