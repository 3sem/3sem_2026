#include "common.h"
#include <limits.h>
#include <pthread.h>

typedef struct {
    Shared *shared;
    size_t index;
} Argument;

/* SplitMix64, Sebastiano Vigna, public domain: prng.di.unimi.it/splitmix64.c */
static double uniform(uint64_t *state)
{
    uint64_t z = (*state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return (double)((z ^ (z >> 31)) >> 11) * 0x1p-53;
}

static void *calculate(void *ptr)
{
    Argument *arg = ptr;
    Shared *s = arg->shared;
    uint64_t base = s->points / s->threads, rest = s->points % s->threads;
    uint64_t count = base + (arg->index < rest);
    uint64_t first = base * arg->index + (arg->index < rest ? arg->index : rest);
    /* Потоки берут разные участки одной последовательности случайных чисел. */
    uint64_t state = s->seed + 2 * first * UINT64_C(0x9e3779b97f4a7c15);
    uint64_t hits = 0;
    for (uint64_t i = 0; i < count; ++i) {
        double x = uniform(&state), y = uniform(&state);
        if (y < x * x)
            ++hits;
    }
    s->hits[arg->index] = hits; /* У каждого потока своя ячейка. */
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Run program b first\n");
        return 1;
    }
    uint64_t id = number(argv[1]);
    if (id > INT_MAX)
        return 1;
    Shared *s = shmat((int)id, NULL, 0);
    if (s == (void *)-1)
        fail("shmat");
    pthread_t *threads = calloc(s->threads, sizeof(*threads));
    Argument *args = calloc(s->threads, sizeof(*args));
    if (!threads || !args)
        fail("calloc");

    double start = now();
    for (size_t i = 0; i < s->threads; ++i) {
        args[i] = (Argument){s, i};
        int error = pthread_create(&threads[i], NULL, calculate, &args[i]);
        if (error) {
            errno = error;
            fail("pthread_create");
        }
    }
    for (size_t i = 0; i < s->threads; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error) {
            errno = error;
            fail("pthread_join");
        }
    }
    s->seconds = now() - start;
    free(args);
    free(threads);
    if (shmdt(s) == -1)
        fail("shmdt");
    return 0;
}
