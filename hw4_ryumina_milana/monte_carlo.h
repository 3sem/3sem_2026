#ifndef MONTE_CARLO_H
#define MONTE_CARLO_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/* Ключи для System V IPC */
#define SHM_KEY_PATH  "monte_carlo_shm"
#define SHM_KEY_ID    42

/* Режимы работы */
#define MODE_INDEPENDENT  0   /* независимые ячейки */
#define MODE_SHARED       1   /* общая ячейка + мьютекс */

/* Максимум потоков */
#define MAX_THREADS 64

/*
 * Разделяемая память между программой А и программой Б.
 * Содержит результаты всех потоков.
 */
struct SharedResults {
    int      num_threads;        /* сколько потоков работало */
    int      mode;               /* MODE_INDEPENDENT или MODE_SHARED */
    double   x_min, x_max;       /* границы области */
    double   y_min, y_max;
    size_t   total_points;       /* всего точек */
    size_t   points_per_thread;  /* точек на поток */
    double   result;             /* итоговый результат (для общей ячейки) */

    /* Массив счётчиков — по одному на поток */
    size_t   counters[MAX_THREADS];

    pthread_mutex_t mutex;       /* мьютекс для общей ячейки */
};

/*
 * Данные, передаваемые в поток.
 */
struct PthreadData {
    double              (*func)(double);
    double              x_min, x_max, y_min, y_max;
    unsigned int        seed;
    size_t              points_per_thread;
    int                 thread_index;      /* номер потока */
    int                 mode;              /* режим */
    struct SharedResults *shared;          /* указатель на shm */
};

double ExponentialFunc(double x);
double CalculateIntegral(double (*func)(double),
                         int num_threads,
                         int mode,
                         double x_min, double x_max,
                         double y_min, double y_max);


int    CreateSharedMemory(size_t size);
void*  AttachSharedMemory(int shmid);
void   DetachSharedMemory(void *ptr);
void   RemoveSharedMemory(int shmid);

#endif // MONTE_CARLO_H