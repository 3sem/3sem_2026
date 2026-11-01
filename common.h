#ifndef COMMON_H
#define COMMON_H

#define MAX_THREADS 64
#define SHM_KEY 0x4242 

typedef struct {
    int num_threads;
    long total_points;
    double results[MAX_THREADS];
} shm_data_t;

#endif