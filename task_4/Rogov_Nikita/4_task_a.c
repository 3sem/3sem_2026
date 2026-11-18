#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "4_task.h"

typedef struct {
    int id;
    long long points;
    shm_data_t* shm;
} thread_arg_t;

double f(double x) {
    return x * x;
}

void* worker(void* arg) {
    thread_arg_t* t = (thread_arg_t*) arg;

    unsigned short seed[3];
    seed[0] = (unsigned short) time(NULL);
    seed[1] = (unsigned short) getpid();
    seed[2] = (unsigned short) (t->id * 7919 + 1);

    long long hits = 0;
    for (long long i = 0; i < t->points; i++) {
        double x = A_BOUND + (B_BOUND - A_BOUND) * erand48(seed);
        double y = F_MAX * erand48(seed);
        if (y < f(x)) {
            hits++;
        }
    }

    t->shm->hits[t->id] = hits;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Использование: %s <N_points> <n_threads>\n", argv[0]);
        return 1;
    }

    long long n_points = atoll(argv[1]);
    int n_threads = atoi(argv[2]);

    if (n_points <= 0 || n_threads <= 0 || n_threads > MAX_THREADS || n_threads > n_points) {
        printf("Нужно: N > 0, 0 < n <= %d, n <= N\n", MAX_THREADS);
        return 1;
    }

    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(shm_data_t)) < 0) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        return 1;
    }

    shm_data_t* shm = mmap(NULL, sizeof(shm_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return 1;
    }
    close(shm_fd);

    shm->ready = 0;
    shm->n_threads = n_threads;
    shm->n_points = n_points;

    pthread_t* threads = calloc(n_threads, sizeof(pthread_t));
    thread_arg_t* args = calloc(n_threads, sizeof(thread_arg_t));
    if (!threads || !args) {
        perror("calloc");
        shm_unlink(SHM_NAME);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int created = 0;
    for (int i = 0; i < n_threads; i++) {
        args[i].id = i;
        args[i].points = n_points / n_threads;
        if (i == n_threads - 1) {
            args[i].points += n_points % n_threads;
        }
        args[i].shm = shm;

        int err = pthread_create(&threads[i], NULL, worker, &args[i]);
        if (err != 0) {
            fprintf(stderr, "pthread_create: %s (создано потоков: %d)\n", strerror(err), created);
            break;
        }
        created++;
    }

    for (int i = 0; i < created; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (created < n_threads) {
        munmap(shm, sizeof(shm_data_t));
        shm_unlink(SHM_NAME);
        free(threads);
        free(args);
        return 1;
    }

    shm->time_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    shm->ready = 1;

    printf("A: N = %lld, n = %d, время = %.4f с\n", n_points, n_threads, shm->time_sec);

    munmap(shm, sizeof(shm_data_t));
    free(threads);
    free(args);
    return 0;
}
