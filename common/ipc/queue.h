#ifndef QUEUE_H
#define QUEUE_H

#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "fileBuffer.h"

void queueSend(key_t key, FileBuffer* buffer);
void queueRead(key_t key, FileBuffer* buffer);

#endif /* QUEUE_H */