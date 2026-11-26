#include "shm.h"

shm_t *creat_shm(){
    shm_t *self = malloc(sizeof(shm_t));
    
    if (!self) {
        perror("SHM mem error");
        return NULL;
    }

    self->key = ftok("/", 'S');
    
    self->shm_id = shmget(self->key, sizeof(buffer_t), IPC_CREAT | 0666);
    if (self->shm_id == -1) {
        perror("SHM error");

        free(self);
        return NULL;
    }

    self->buffer = (buffer_t *)shmat(self->shm_id, NULL, 0);

    if (self->buffer == (void *)-1) {

        perror("mem connect error");

        shmctl(self->shm_id, IPC_RMID, NULL);
        free(self);
        return NULL;
    }

    
    self->buffer->capacity = MAX_BUF_SIZE;
    self->buffer->actions  = &default_buf_ops;

    return self;
}

void destroy_shm(shm_t *self){
    if (!self) return;

    if (shmdt(self->buffer) == -1) {
        perror("SHM destroy error");
    }

    if (shmctl(self->shm_id, IPC_RMID, NULL) == -1) {
        perror("SHM destroy error");
    }

    free(self);
}