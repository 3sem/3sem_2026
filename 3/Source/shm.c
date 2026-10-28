#ifdef SHM
#include "shm.h"


void start_parent(shm_t *shm, sem_t *sem){
    int output_fd = open(destination, O_WRONLY | O_CREAT, 0644);

    if (output_fd == -1) {
        perror("open source error");
        return;
    }
    int i = 0;

    while (1) {
        i = (i + 1) % BUF_NUM;

        sem->sem_ops->wait_reader(*sem);

        if (shm->buffer[i]->size == 0) break;
        shm->buffer[i]->actions->read_from_buffer(shm->buffer[i], output_fd);

        sem->sem_ops->open_writer(*sem);
    }

    close(output_fd);
}

void start_child(shm_t *shm, sem_t *sem){
    int input_fd = open(source, O_RDONLY);

    if (input_fd == -1) {
        perror("open source error");
        return;
    }
    int i = 0;

    do {
        i = (i + 1) % BUF_NUM;

        sem->sem_ops->wait_writer(*sem);

        shm->buffer[i]->actions->write_in_buffer(shm->buffer[i], input_fd);
        
        sem->sem_ops->open_reader(*sem);
    } while (shm->buffer[i]->size > 0);

    close(input_fd);
}

shm_t *creat_shm(){
    shm_t *self = malloc(sizeof(shm_t));
    
    if (!self) {
        perror("SHM mem error");
        return NULL;
    }

    self->key = ftok("/", 'S');
    
    self->shm_id = shmget(self->key, sizeof(buffer_t) * BUF_NUM, IPC_CREAT | 0666);
    if (self->shm_id == -1) {
        perror("SHM error");

        free(self);
        return NULL;
    }

    self->buffer[0] = (buffer_t *)shmat(self->shm_id, NULL, 0);

    if (self->buffer[0] == (void *)-1) {

        perror("mem connect error");

        shmctl(self->shm_id, IPC_RMID, NULL);
        free(self);
        return NULL;
    }

    
    for (int i = 0; i < 2; i++){
        self->buffer[i] = self->buffer[0] + i;
        self->buffer[i]->capacity = MAX_BUF_SIZE;
        self->buffer[i]->actions  = &default_buf_ops;
    }

    return self;
}

void destroy_shm(shm_t *self){
    if (!self) return;

    if (shmdt(self->buffer[0]) == -1) {
        perror("SHM destroy error");
    }

    if (shmctl(self->shm_id, IPC_RMID, NULL) == -1) {
        perror("SHM destroy error");
    }

    free(self);
}

#endif