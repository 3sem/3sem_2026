#ifndef QUEUE_H
#define QUEUE_H

#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "fileBuffer.h"

int queueSend(key_t key, FileBuffer* buffer, size_t chunkSize);
int queueRead(key_t key, FileBuffer* buffer, size_t chunkSize);

#endif /* QUEUE_H */