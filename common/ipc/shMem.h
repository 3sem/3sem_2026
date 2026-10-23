#ifndef SH_MEM_H
#define SH_MEM_H

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "fileBuffer.h"

int shMemSend(key_t key, FileBuffer* buffer, size_t chunkSize);
int shMemRead(key_t key, FileBuffer* buffer, size_t chunkSize);

int shMemSendString(key_t key, const char* string, size_t chunkSize);

int shMemReadString(key_t key, char* string, size_t stringSize, size_t chunkSize);

#endif /* SH_MEM_H */
