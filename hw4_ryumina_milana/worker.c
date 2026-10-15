#define _GNU_SOURCE
#include <math.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "monte_carlo.h"

const size_t TOTAL_POINTS = 10000000;

double ExponentialFunc(double x) {
    return exp(x);
}

/* ---------- Разделяемая память ---------- */

int CreateSharedMemory(size_t size) {
    key_t key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
    if (key == -1) {
        /* Если файла нет — создадим временный */
        FILE *f = fopen(SHM_KEY_PATH, "w");
        if (f) fclose(f);
        key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
    }
    if (key == -1) {
        perror("ftok");
        return -1;
    }

    int shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1 && errno == EEXIST) {
        shmid = shmget(key, size, 0666);
    }
    if (shmid == -1) {
        perror("shmget");
    }
    return shmid;
}

void* AttachSharedMemory(int shmid) {
    void *ptr = shmat(shmid, NULL, 0);
    if (ptr == (void*)-1) {
        perror("shmat");
        return NULL;
    }
    return ptr;
}

void DetachSharedMemory(void *ptr) {
    if (ptr) {
        shmdt(ptr);
    }
}

void RemoveSharedMemory(int shmid) {
    shmctl(shmid, IPC_RMID, NULL);
}

/* ---------- Поток ---------- */

static void* MonteCarloThread(void *args) {
    assert(args);

    struct PthreadData *data = (struct PthreadData*)args;
    size_t local_count = 0;

    /* Локальный ГПСЧ — не мешает другим потокам */
    for (size_t i = 0; i < data->points_per_thread; i++) {
        double x = data->x_min + (double)rand_r(&data->seed) / RAND_MAX * (data->x_max - data->x_min);
        double y = data->y_min + (double)rand_r(&data->seed) / RAND_MAX * (data->y_max - data->y_min);

        if (y <= data->func(x)) {
            local_count++;
        }
    }

    if (data->mode == MODE_INDEPENDENT) {
        /* Независимая ячейка — без синхронизации */
        data->shared->counters[data->thread_index] = local_count;
    } else {
        /* Общая ячейка — с мьютексом */
        pthread_mutex_lock(&data->shared->mutex);
        data->shared->counters[0] += local_count;
        pthread_mutex_unlock(&data->shared->mutex);
    }

    return NULL;
}

/* ---------- Основная логика ---------- */

double CalculateIntegral(double (*func)(double),
                         int num_threads,
                         int mode,
                         double x_min, double x_max,
                         double y_min, double y_max) {
    /* Создаём разделяемую память */
    int shmid = CreateSharedMemory(sizeof(struct SharedResults));
    if (shmid == -1) {
        return 0.0;
    }

    struct SharedResults *shared = (struct SharedResults*)AttachSharedMemory(shmid);
    if (shared == NULL) {
        RemoveSharedMemory(shmid);
        return 0.0;
    }

    /* Инициализация */
    memset(shared, 0, sizeof(*shared));
    shared->num_threads = num_threads;
    shared->mode = mode;
    shared->x_min = x_min;
    shared->x_max = x_max;
    shared->y_min = y_min;
    shared->y_max = y_max;
    shared->total_points = TOTAL_POINTS;
    shared->points_per_thread = TOTAL_POINTS / (size_t)num_threads;

    /* Мьютекс в разделяемой памяти — для режима SHARED */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    struct PthreadData *data = calloc((size_t)num_threads, sizeof(*data));
    pthread_t *tid = calloc((size_t)num_threads, sizeof(*tid));

    struct timespec start = {}, end = {};
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Запуск потоков */
    for (int i = 0; i < num_threads; i++) {
        data[i].x_min = x_min;
        data[i].x_max = x_max;
        data[i].y_min = y_min;
        data[i].y_max = y_max;
        data[i].func = func;
        data[i].points_per_thread = shared->points_per_thread;
        data[i].seed = (unsigned int)time(NULL) + (unsigned int)i * 7919u;
        data[i].thread_index = i;
        data[i].mode = mode;
        data[i].shared = shared;

        pthread_create(&tid[i], NULL, MonteCarloThread, &data[i]);
    }

    /* Ждём завершения */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(tid[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (double)(end.tv_sec - start.tv_sec) +
                      (double)(end.tv_nsec - start.tv_nsec) / 1e9;

    /* Суммируем результаты — для режима INDEPENDENT */
    if (mode == MODE_INDEPENDENT) {
        size_t total = 0;
        for (int i = 0; i < num_threads; i++) {
            total += shared->counters[i];
        }
        shared->result = (x_max - x_min) * (y_max - y_min) * (double)total / (double)TOTAL_POINTS;
    } else {
        shared->result = (x_max - x_min) * (y_max - y_min) * (double)shared->counters[0] / (double)TOTAL_POINTS;
    }

    printf("Time duration: %lg\n", duration);
    printf("Result: %.10f\n", shared->result);
    printf("shmid=%d (для программы Б)\n", shmid);

    double final_result = shared->result;
    pthread_mutex_destroy(&shared->mutex);
    DetachSharedMemory(shared);
    free(data);
    free(tid);

    /* НЕ удаляем shm — программа Б должна прочитать */
    return final_result;
}