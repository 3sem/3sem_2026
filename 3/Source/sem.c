#ifdef SHM
#include "sem.h"

static struct sembuf reader_signal = {SEM_READ ,  1, 0};
static struct sembuf reader_wait   = {SEM_READ , -1, 0};
static struct sembuf writer_signal = {SEM_WRITE,  1, 0};
static struct sembuf writer_wait   = {SEM_WRITE, -1, 0};

void open_reader (sem_t self){
    semop(self.sem, &reader_signal, 1);
}

void wait_reader(sem_t self) {
    semop(self.sem, &reader_wait, 1);
}

void open_writer(sem_t self) {
    semop(self.sem, &writer_signal, 1);
}

void wait_writer(sem_t self) {
    semop(self.sem, &writer_wait, 1);
}

sem_t *create_sem(){
    sem_t* sem = malloc(sizeof(sem_t));
    
    if (!sem){
        perror("Memory error");
        return NULL;    
    }
    sem->key = ftok("/", 'W');

    sem->sem = semget(sem->key, 2, IPC_CREAT | 0666);

    if (semctl(sem->sem, SEM_WRITE, SETVAL, 1) == -1) {
        perror("semctl CHILD");
        return NULL;
    }

    if (semctl(sem->sem, SEM_READ, SETVAL, 0) == -1) {
        perror("semctl PARENT");
        return NULL;
    }

    sem->sem_ops = &default_sem_ops;

    return sem;
}

void destroy_sem(sem_t *self){
    if (!self) return;

    if (semctl(self->sem, 0, IPC_RMID) == -1) {
        perror("destroy error");
    }

    free(self);
}

#endif