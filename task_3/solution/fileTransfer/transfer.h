#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stddef.h>

int send(const char* inputFileName, size_t chunkSize);
int receive(const char* outputFileName, size_t chunkSize);

#endif /* FILE_TRANSFER_H */
