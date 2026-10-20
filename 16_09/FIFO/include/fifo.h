#ifndef FIFO_H
#define FIFO_H

#include <stddef.h>

#define FIFO_PATH "transfer.fifo"

int prepareFifo(const char *path);
int openFifoForReading(const char *path);
int openFifoForWriting(const char *path);
int writeAll(int fd, const void *buffer, size_t size);
int removeFifo(const char *path);

#endif
