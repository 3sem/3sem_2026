#ifndef TASK4_SHARED_H
#define TASK4_SHARED_H

#define SHM_NAME    "/monte_carlo_shm"
#define MAX_THREADS 4096

#define A_BOUND 0.0
#define B_BOUND 2.0
#define F_MAX   4.0

typedef struct {
    int ready;
    int n_threads;
    long long n_points;
    double time_sec;
    long long hits[MAX_THREADS];
} shm_data_t;

#endif
