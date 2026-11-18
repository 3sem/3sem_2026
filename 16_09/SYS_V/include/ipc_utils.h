#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include <sys/ipc.h>

key_t getIpcKey(const char *path, int project_id);

int waitSemaphore(int semid, unsigned short sem_num);
int postSemaphore(int semid, unsigned short sem_num);

int detachSharedMemory(void *data);
int removeSharedMemory(int shmid);
int removeSemaphoreSet(int semid);
int removeMessageQueue(int msqid);

#endif
