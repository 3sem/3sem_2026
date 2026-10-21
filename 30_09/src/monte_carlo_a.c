#define _DEFAULT_SOURCE

#include "monte_carlo_common.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <unistd.h>

typedef struct {
    mc_shared_data_t *shared;
    uint64_t point_count;
    struct drand48_data random_state;
    size_t thread_index;
    mc_mode_t mode;
    int error;
} thread_context_t;

static int nextUnitInterval(struct drand48_data *state, double *value) {
    return drand48_r(state, value);
}

static void *calculateHits(void *argument) {
    thread_context_t *context = argument;
    uint64_t local_hits = 0;

    for (uint64_t i = 0; i < context->point_count; ++i) {
        double x;
        double y;

        int error = nextUnitInterval(&context->random_state, &x);
        if (error != 0) {
            context->error = error;
            return NULL;
        }

        error = nextUnitInterval(&context->random_state, &y);
        if (error != 0) {
            context->error = error;
            return NULL;
        }

        if (y <= x) {
            if (context->mode == MC_MODE_PER_THREAD) {
                ++local_hits;
            } 
            else {
                error = pthread_mutex_lock(&context->shared->mutex);
                if (error != 0) {
                    context->error = error;
                    return NULL;
                }

                ++context->shared->total_hits;

                error = pthread_mutex_unlock(&context->shared->mutex);
                if (error != 0) {
                    context->error = error;
                    return NULL;
                }
            }
        }
    }

    if (context->mode == MC_MODE_PER_THREAD) {
        context->shared->thread_hits[context->thread_index] = local_hits;
    }

    return NULL;
}

static void destroyMutexIfNeeded(
    mc_shared_data_t *shared,
    bool initialized
) {
    if (initialized) {
        int error = pthread_mutex_destroy(&shared->mutex);
        if (error != 0) {
            fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(error));
        }
    }
}

static void removeFailedSegment(int shmid, mc_shared_data_t *shared) {
    if (shared != NULL && shared != (void *)-1 && shmdt(shared) == -1) {
        perror("shmdt");
    }

    if (shmid >= 0 && shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}

static int parseArguments(
    int argc,
    char *argv[],
    uint64_t *total_points,
    uint64_t *requested_threads,
    mc_mode_t *mode
) {
    if (argc != 4) {
        fprintf(
            stderr,
            "Usage: %s <total_points> <threads> "
            "<per_thread|shared_mutex>\n",
            argv[0]
        );
        return -1;
    }

    if (parseUint64(argv[1], total_points) == -1 ||
        parseUint64(argv[2], requested_threads) == -1 ||
        parseMode(argv[3], mode) == -1) {
        fprintf(
            stderr,
            "Invalid arguments. Points and threads must be positive "
            "integers; mode must be per_thread or shared_mutex.\n"
        );
        return -1;
    }

    if (*requested_threads > *total_points ||
        *requested_threads > SIZE_MAX / sizeof(pthread_t) ||
        *requested_threads > SIZE_MAX / sizeof(thread_context_t)) {
        fprintf(stderr, "Thread count is too large for the requested workload or platform.\n");
        return -1;
    }

    return 0;
}

static int createSharedMemory(
    uint64_t total_points,
    size_t thread_count,
    mc_mode_t mode,
    int *shmid_out,
    mc_shared_data_t **shared_out,
    bool *mutex_initialized
) {
    size_t data_offset = offsetof(mc_shared_data_t, thread_hits);
    if (thread_count > (SIZE_MAX - data_offset) / sizeof(uint64_t)) {
        fprintf(stderr, "Shared memory size overflow.\n");
        return -1;
    }

    size_t shared_size = data_offset + thread_count * sizeof(uint64_t);

    key_t key = getIpcKey();
    if (key == (key_t)-1) {
        return -1;
    }

    int shmid = shmget(key, shared_size, IPC_CREAT | IPC_EXCL | 0600);
    if (shmid == -1) {
        if (errno == EEXIST) {
            fprintf(
                stderr,
                "A shared-memory segment already exists. "
                "Run './monte_carlo_b --cleanup' and retry.\n"
            );
        } 
        else {
            perror("shmget");
        }
        return -1;
    }

    void *memory = shmat(shmid, NULL, 0);
    if (memory == (void *)-1) {
        perror("shmat");
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID");
        }
        return -1;
    }

    mc_shared_data_t *shared = memory;
    memset(shared, 0, shared_size);
    shared->total_points = total_points;
    shared->thread_count = (uint64_t)thread_count;
    shared->mode = (uint32_t)mode;

    *mutex_initialized = false;
    if (mode == MC_MODE_SHARED_MUTEX) {
        int error = pthread_mutex_init(&shared->mutex, NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_mutex_init: %s\n", strerror(error));
            removeFailedSegment(shmid, shared);
            return -1;
        }
        *mutex_initialized = true;
    }

    *shmid_out = shmid;
    *shared_out = shared;
    return 0;
}

static int initializeThreadContexts(
    thread_context_t *contexts,
    mc_shared_data_t *shared,
    uint64_t total_points,
    size_t thread_count,
    mc_mode_t mode
) {
    struct timespec seed_time;
    if (clock_gettime(CLOCK_MONOTONIC, &seed_time) == -1) {
        perror("clock_gettime seed");
        return -1;
    }

    uint64_t base_points = total_points / (uint64_t)thread_count;
    uint64_t remainder = total_points % (uint64_t)thread_count;
    uint64_t seed_base = ((uint64_t)seed_time.tv_sec << 32) ^
                         (uint64_t)seed_time.tv_nsec ^
                         ((uint64_t)getpid() << 16);

    for (size_t i = 0; i < thread_count; ++i) {
        contexts[i].shared = shared;
        contexts[i].thread_index = i;
        contexts[i].mode = mode;
        contexts[i].point_count = base_points + ((uint64_t)i < remainder ? 1 : 0);
        contexts[i].error = 0;

        uint64_t raw_seed = seed_base ^
            (UINT64_C(0x9e3779b97f4a7c15) * (uint64_t)(i + 1));
        long seed = (long)(raw_seed & UINT64_C(0x7fffffff));
        if (seed == 0) {
            seed = (long)(i + 1);
        }

        int error = srand48_r(seed, &contexts[i].random_state);
        if (error != 0) {
            fprintf(stderr, "srand48_r: %s\n", strerror(error));
            return -1;
        }
    }

    return 0;
}

static int runThreads(
    pthread_t *threads,
    thread_context_t *contexts,
    size_t thread_count,
    double *elapsed_seconds
) {
    struct timespec start;
    struct timespec stop;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime start");
        return -1;
    }

    size_t created_threads = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        int error = pthread_create(
            &threads[i],
            NULL,
            calculateHits,
            &contexts[i]
        );

        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            for (size_t j = 0; j < created_threads; ++j) {
                int join_error = pthread_join(threads[j], NULL);
                if (join_error != 0) {
                    fprintf(stderr, "pthread_join: %s\n", strerror(join_error));
                }
            }
            return -1;
        }

        ++created_threads;
    }

    int join_failed = 0;
    for (size_t i = 0; i < created_threads; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            join_failed = 1;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
        perror("clock_gettime stop");
        return -1;
    }

    int worker_failed = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        if (contexts[i].error != 0) {
            fprintf(
                stderr,
                "Worker thread failed: %s\n",
                strerror(contexts[i].error)
            );
            worker_failed = 1;
        }
    }

    if (join_failed || worker_failed) {
        return -1;
    }

    *elapsed_seconds = (double)(stop.tv_sec - start.tv_sec) +
        (double)(stop.tv_nsec - start.tv_nsec) / 1e9;

    return 0;
}

int main(int argc, char *argv[]) {
    uint64_t total_points;
    uint64_t requested_threads;
    mc_mode_t mode;

    if (parseArguments(
            argc,
            argv,
            &total_points,
            &requested_threads,
            &mode
        ) == -1) {
        return 1;
    }

    size_t thread_count = (size_t)requested_threads;
    int shmid = -1;
    mc_shared_data_t *shared = NULL;
    bool mutex_initialized = false;

    if (createSharedMemory(
            total_points,
            thread_count,
            mode,
            &shmid,
            &shared,
            &mutex_initialized
        ) == -1) {
        return 1;
    }

    pthread_t *threads = calloc(thread_count, sizeof(*threads));
    thread_context_t *contexts = calloc(thread_count, sizeof(*contexts));
    if (threads == NULL || contexts == NULL) {
        perror("calloc thread data");
        free(threads);
        free(contexts);
        destroyMutexIfNeeded(shared, mutex_initialized);
        removeFailedSegment(shmid, shared);
        return 1;
    }

    if (initializeThreadContexts(
            contexts,
            shared,
            total_points,
            thread_count,
            mode
        ) == -1) {
        free(threads);
        free(contexts);
        destroyMutexIfNeeded(shared, mutex_initialized);
        removeFailedSegment(shmid, shared);
        return 1;
    }

    double elapsed_seconds;
    if (runThreads(
            threads,
            contexts,
            thread_count,
            &elapsed_seconds
        ) == -1) {
        free(threads);
        free(contexts);
        destroyMutexIfNeeded(shared, mutex_initialized);
        removeFailedSegment(shmid, shared);
        return 1;
    }

    free(threads);
    free(contexts);

    if (mutex_initialized) {
        int error = pthread_mutex_destroy(&shared->mutex);
        if (error != 0) {
            fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(error));
            removeFailedSegment(shmid, shared);
            return 1;
        }
    }

    shared->ready = 1;
    if (shmdt(shared) == -1) {
        perror("shmdt");
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID");
        }
        return 1;
    }

    printf(
        "TIME,%s,%" PRIu64 ",%" PRIu64 ",%.9f\n",
        modeName(mode),
        requested_threads,
        total_points,
        elapsed_seconds
    );

    return 0;
}
