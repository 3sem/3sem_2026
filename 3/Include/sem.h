#ifndef SEM_H_
#define SEM_H_

#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>

#define BUF_NUM 2

#define SEM_WRITE 0
#define SEM_READ  1

typedef struct Sem sem_t;

typedef struct {
    void (*open_reader)(sem_t);
    void (*wait_reader)(sem_t);
    void (*open_writer)(sem_t);
    void (*wait_writer)(sem_t);
} sem_ops;

struct Sem{
    int key;
    int sem;
    const sem_ops *sem_ops;
};

void open_reader(sem_t self);
void wait_reader(sem_t self);
void open_writer(sem_t self);
void wait_writer(sem_t self);

static const sem_ops default_sem_ops = {
    .open_reader = open_reader ,
    .wait_reader = wait_reader,
    .open_writer = open_writer ,
    .wait_writer = wait_writer,
};

sem_t *create_sem();
void destroy_sem(sem_t *self);

#endif