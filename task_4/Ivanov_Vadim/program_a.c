#define _POSIX_C_SOURCE 200809L

#include "shared.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    ThreadResult *result;
    uint64_t seed;
} Worker;

static int parse_number(const char *text, uint64_t *value)
{
    char *end;
    errno = 0;
    if (text[0] < '0' || text[0] > '9')
        return -1;
    *value = strtoull(text, &end, 10);
    return errno || *end ? -1 : 0;
}

static uint64_t random_next(uint64_t *state)
{
    *state += UINT64_C(0x9e3779b97f4a7c15);
    uint64_t value = *state;
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static void *calculate(void *argument)
{
    Worker *worker = argument;
    uint64_t state = worker->seed;
    uint64_t hits = 0;
    uint64_t points = worker->result->points;

    for (uint64_t point = 0; point < points; ++point) {
        double horizontal = (random_next(&state) >> 11) * 0x1.0p-53;
        double vertical = (random_next(&state) >> 11) * 0x1.0p-53;
        if (vertical <= horizontal * horizontal)
            ++hits;
    }
    worker->result->hits = hits;
    return NULL;
}

int main(int argc, char **argv)
{
    uint64_t thread_count;
    uint64_t points;
    uint64_t seed = (uint64_t)time(NULL);
    const char *name = argc > 3 ? argv[3] : DEFAULT_SHM_NAME;

    if (argc < 3 || argc > 5 || parse_number(argv[1], &thread_count) ||
        parse_number(argv[2], &points) || thread_count == 0 ||
        thread_count > 4096 || points < thread_count ||
        (argc == 5 && parse_number(argv[4], &seed))) {
        fprintf(stderr, "Usage: %s THREADS POINTS [SHM_NAME [SEED]]\n", argv[0]);
        fprintf(stderr, "THREADS: 1..4096; POINTS >= THREADS\n");
        return 1;
    }

    int descriptor = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (descriptor == -1) {
        perror("shm_open (read the previous result with program_b first)");
        return 1;
    }
    size_t bytes = sizeof(SharedResults) + (size_t)thread_count * sizeof(ThreadResult);
    if (ftruncate(descriptor, (off_t)bytes) == -1) {
        perror("ftruncate");
        close(descriptor);
        shm_unlink(name);
        return 1;
    }
    SharedResults *shared = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, descriptor, 0);
    close(descriptor);
    if (shared == MAP_FAILED) {
        perror("mmap");
        shm_unlink(name);
        return 1;
    }
    atomic_store_explicit(&shared->ready, 0, memory_order_relaxed);
    shared->threads = (unsigned int)thread_count;
    shared->points = points;
    shared->seed = seed;

    pthread_t *threads = calloc((size_t)thread_count, sizeof(*threads));
    Worker *workers = calloc((size_t)thread_count, sizeof(*workers));
    if (!threads || !workers) {
        perror("calloc");
        free(threads);
        free(workers);
        munmap(shared, bytes);
        shm_unlink(name);
        return 1;
    }

    struct timespec start;
    struct timespec finish;
    clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned int started = 0;
    for (unsigned int index = 0; index < shared->threads; ++index) {
        shared->results[index].points = points / thread_count +
                                        (index < points % thread_count);
        workers[index].result = &shared->results[index];
        workers[index].seed = seed + UINT64_C(0xd1b54a32d192ed03) * (index + 1);
        int error = pthread_create(&threads[index], NULL, calculate, &workers[index]);
        if (error) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            break;
        }
        ++started;
    }
    for (unsigned int index = 0; index < started; ++index)
        pthread_join(threads[index], NULL);
    clock_gettime(CLOCK_MONOTONIC, &finish);
    shared->seconds = (double)(finish.tv_sec - start.tv_sec) +
                      (double)(finish.tv_nsec - start.tv_nsec) / 1e9;

    int success = started == shared->threads;
    if (success) {
        atomic_store_explicit(&shared->ready, 1, memory_order_release);
        printf("Results saved. Run: ./program_b %s\n", name);
    } else {
        shm_unlink(name);
    }
    free(threads);
    free(workers);
    munmap(shared, bytes);
    return success ? 0 : 1;
}
