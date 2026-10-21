#ifndef SHARED_H
#define SHARED_H

#include <stdatomic.h>
#include <stdint.h>

#define DEFAULT_SHM_NAME "/kt_monte_carlo"

typedef struct {
    uint64_t points;
    uint64_t hits;
} ThreadResult;

typedef struct {
    atomic_uint ready;
    unsigned int threads;
    uint64_t points;
    uint64_t seed;
    double seconds;
    ThreadResult results[];
} SharedResults;

#endif
