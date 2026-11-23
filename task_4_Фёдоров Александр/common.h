#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

typedef struct {
    uint64_t points, seed;
    size_t threads;
    double seconds;
    uint64_t hits[];
} Shared;

static inline void fail(const char *message)
{
    perror(message);
    exit(1);
}

static inline uint64_t number(const char *text)
{
    char *end;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno || *text < '0' || *text > '9' || *end || value > UINT64_MAX) {
        fprintf(stderr, "Invalid number: %s\n", text);
        exit(1);
    }
    return (uint64_t)value;
}

static inline double now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == -1)
        fail("clock_gettime");
    return (double)t.tv_sec + t.tv_nsec / 1e9;
}
#endif
