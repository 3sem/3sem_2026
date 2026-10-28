#ifndef RESULT_TRANSFER_H
#define RESULT_TRANSFER_H

#include <stddef.h>

#define RESULT_SHMEM_PATH "/tmp/integralShm"
#define RESULT_STRING_SIZE 4096

int sendResult(const char* expression, const double borders[2],
               const double result[2], size_t chunkSize);
int receiveResult(size_t chunkSize);

#endif /* RESULT_TRANSFER_H */
