#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <pthread.h>
#include <stddef.h>

#define MC_SHM_NAME "/mc_integral_shm"
#define MAX_SHM_THREADS 1024

typedef double (* MathFunc)(double);

typedef struct {
    double minX;
    double maxX;
    double maxY;
    MathFunc func;
    unsigned int seed;
    int thread_num;
    size_t dots_count;
    double out_value;
    double *shared_sum;
    pthread_mutex_t *mutex;
} Calculator;

typedef struct {
    int threads_count;
    size_t total_dots;
    int sync_mode;
    double elapsed_sec;
    double shared_total;
    double partial_sums[MAX_SHM_THREADS];
} SharedResult;

double Func(double x);

void *Calculate(void *arg);

double now_sec(void);

#endif /* CALCULATOR_H */
