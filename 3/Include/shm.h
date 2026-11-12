#ifndef SHM_H_
#define SHM_H_

#include "buffer.h"
#include "sem.h"
#include <sys/shm.h>

typedef struct {
    key_t     key;
    int       shm_id;
    buffer_t *buffer[BUF_NUM];
} shm_t;

shm_t *creat_shm();
void   destroy_shm(shm_t *self);

void start_parent(shm_t *shm, sem_t *sem);
void start_child (shm_t *shm, sem_t *sem);

#endif