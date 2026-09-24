#ifndef SH_MEM_H
#define SH_MEM_H

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "fileBuffer.h"

void shMemSend(key_t key, FileBuffer* buffer);
void shMemRead(key_t key, FileBuffer* buffer);

#endif /* SH_MEM_H */