#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sched.h>
#include "common.h"

typedef struct {
    int thread_id;
    long points_per_thread;
    shm_data_t *shm_ptr;
} thread_arg_t;

void* monte_carlo_worker(void* arg) {
    thread_arg_t *t_arg = (thread_arg_t*)arg;
    
    unsigned int seed = time(NULL) ^ (t_arg->thread_id * 1337); 
    long hits = 0;

    for (long i = 0; i < t_arg->points_per_thread; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        
        if (y <= x * x) {
            hits++;
        }
    }

    t_arg->shm_ptr->results[t_arg->thread_id] = (double)hits / t_arg->points_per_thread;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <total_points> <num_threads>\n", argv[0]);
        return 1;
    }

    long total_points = atol(argv[1]);
    int num_threads = atoi(argv[2]);

    if (num_threads > MAX_THREADS || num_threads < 1) {
        printf("Threads must be between 1 and %d\n", MAX_THREADS);
        return 1;
    }

    int shmid = shmget(SHM_KEY, sizeof(shm_data_t), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    shm_data_t *shm_ptr = (shm_data_t*)shmat(shmid, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("shmat");
        return 1;
    }

    shm_ptr->num_threads = num_threads;
    shm_ptr->total_points = total_points;

    pthread_t threads[MAX_THREADS];
    thread_arg_t args[MAX_THREADS];
    long points_per_thread = total_points / num_threads;

    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].points_per_thread = points_per_thread;
        args[i].shm_ptr = shm_ptr;
        pthread_create(&threads[i], NULL, monte_carlo_worker, &args[i]);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i % num_cores, &cpuset);
        pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[ProgA] Points: %ld, Threads: %d, Time: %.4f s\n", total_points, num_threads, elapsed);

    shmdt(shm_ptr);
    return 0;
}