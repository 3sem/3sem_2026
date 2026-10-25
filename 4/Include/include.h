#ifndef INCLUDE_H_
#define INCLUDE_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>   // cpu_set_t, CPU_SET
#include <pthread.h>

#include "shm.h"
#include "int.h"

#define PTHREADS_NUM      100

#define POINTS_NUM        (16000000)
#define POINTS_IN_PTHREAD (POINTS_NUM / PTHREADS_NUM)

typedef struct {
    points_params_t points;
    func_t user_func;
    sem_t *sem;
    shm_t *shm;
    int    pthread_id;
} thread_args_t;

static const int max_cpu = 16;
pthread_t pthreads[PTHREADS_NUM];

int create_pthreads(sem_t *sem, shm_t *shm);
void *check_points_set(void *thread_args);

#endif