#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include "shared_data.h"

static double f(double x) { 
    return x * x * x; 
}

struct ThreadArgs {
    int thread_id;
    long long points_per_thread;
    struct SharedData* shm_ptr;
};

void* worker(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    
    // (CPU AFFINITY)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores <= 0) num_cores = 1;

    CPU_SET(args->thread_id % num_cores, &cpuset); 
    
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

    struct drand48_data rand_buffer;
    unsigned short xsubi[3];
    xsubi[0] = time(NULL) & 0xFFFF;
    xsubi[1] = getpid() & 0xFFFF;
    xsubi[2] = args->thread_id & 0xFFFF;
    seed48_r(xsubi, &rand_buffer);

    long long local_count = 0;
    double rand_x, rand_y;

    for (long long i = 0; i < args->points_per_thread; ++i) {
        drand48_r(&rand_buffer, &rand_x);
        drand48_r(&rand_buffer, &rand_y);

        double x = rand_x * 2.0; // Интервал [0, 2]
        double y = rand_y * 8.0; // Область значений [0, 8]

        if (y <= f(x)) {
            local_count++;
        }
    }

    args->shm_ptr->local_hits[args->thread_id] = local_count;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <N_всего> <n_потоков>\n", argv[0]);
        return 1;
    }

    long long N = atoll(argv[1]);
    int n = atoi(argv[2]);

    if (n > MAX_THREADS) {
        fprintf(stderr, "Ошибка: Максимум %d потоков\n", MAX_THREADS);
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        fprintf(stderr, "Ошибка: SHM не найден! Сначала запустите Программу Б.\n");
        return 1;
    }

    struct SharedData* shm_ptr = (struct SharedData*)mmap(0, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    long long points_per_thread = N / n;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t* threads = malloc(n * sizeof(pthread_t));
    struct ThreadArgs* t_args = malloc(n * sizeof(struct ThreadArgs));

    for (int i = 0; i < n; ++i) {
        t_args[i].thread_id = i;
        t_args[i].points_per_thread = points_per_thread;
        t_args[i].shm_ptr = shm_ptr;
        pthread_create(&threads[i], NULL, worker, &t_args[i]);
    }

    for (int i = 0; i < n; ++i) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("%f\n", elapsed);

    free(threads);
    free(t_args);
    munmap(shm_ptr, sizeof(struct SharedData));
    close(shm_fd);
    return 0;
}
