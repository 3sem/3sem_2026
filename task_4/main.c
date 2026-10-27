#define _GNU_SOURCE

#include "calculator.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <threads_count> <dots_count> [--sync] [--verbose]\n", argv[0]);
        return 1;
    }

    int threads_count = (int)strtol(argv[1], NULL, 10);
    long long dots_ll = strtoll(argv[2], NULL, 10);
    if (threads_count <= 0 || dots_ll <= 0) {
        fprintf(stderr, "Error: threads_count and dots_count must be positive\n");
        return 1;
    }
    if (threads_count > MAX_SHM_THREADS) {
        threads_count = MAX_SHM_THREADS;
    }
    size_t dots_count = (size_t)dots_ll;

    int sync_mode = 0;
    int verbose = 0;
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--sync") == 0) {
            sync_mode = 1;
        } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        }
    }

    /* Открываем сегмент разделяемой памяти (POSIX shared memory) */
    int shm_fd = shm_open(MC_SHM_NAME, O_CREAT | O_RDWR, 0666);
    SharedResult *shm_ptr = NULL;
    if (shm_fd >= 0) {
        if (ftruncate(shm_fd, sizeof(SharedResult)) == 0) {
            void *mapped = mmap(NULL, sizeof(SharedResult),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (mapped != MAP_FAILED) {
                shm_ptr = (SharedResult *)mapped;
                memset(shm_ptr, 0, sizeof(SharedResult));
                shm_ptr->threads_count = threads_count;
                shm_ptr->total_dots = dots_count;
                shm_ptr->sync_mode = sync_mode;
            }
        }
        close(shm_fd);
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    double shared_hits = 0.0;

    double start = now_sec();

    pthread_t *threads = (pthread_t *)calloc((size_t)threads_count, sizeof(pthread_t));
    Calculator *calc = (Calculator *)calloc((size_t)threads_count, sizeof(Calculator));
    if (!threads || !calc) {
        perror("calloc");
        free(threads);
        free(calc);
        return 1;
    }

    unsigned int base_seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
    const double total_span_x = 10.0;
    const double max_y = 1.0;
    size_t base_dots = dots_count / (size_t)threads_count;
    size_t rem_dots = dots_count % (size_t)threads_count;

    for (int i = 0; i < threads_count; i++) {
        calc[i].minX = (total_span_x / threads_count) * i;
        calc[i].maxX = (total_span_x / threads_count) * (i + 1);
        calc[i].maxY = max_y;
        calc[i].func = Func;
        calc[i].thread_num = i;
        calc[i].seed = base_seed ^ (unsigned int)((i + 1) * 2654435761u);
        calc[i].dots_count = base_dots + ((size_t)i < rem_dots ? 1 : 0);
        calc[i].out_value = 0.0;
        if (sync_mode) {
            calc[i].shared_sum = &shared_hits;
            calc[i].mutex = &mutex;
        } else {
            calc[i].shared_sum = NULL;
            calc[i].mutex = NULL;
        }
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_create(&threads[i], NULL, Calculate, &calc[i]);
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Записываем независимые результаты потоков в разделяемую память */
    double summ = 0.0;
    if (sync_mode) {
        summ = (shared_hits / (double)dots_count) * total_span_x * max_y;
        if (shm_ptr) {
            shm_ptr->shared_total = summ;
        }
    } else {
        for (int i = 0; i < threads_count; i++) {
            if (shm_ptr) {
                shm_ptr->partial_sums[i] = calc[i].out_value;
            }
            summ += calc[i].out_value;
        }
    }

    double end = now_sec();
    double seconds = end - start;

    if (shm_ptr) {
        shm_ptr->elapsed_sec = seconds;
    }

    if (verbose) {
        fprintf(stderr, "Integral [0..10] 1/(x+1) dx = %.6f (exact ln(11) = 2.397895), threads = %d, time = %.6f s\n",
                summ, threads_count, seconds);
    }

    printf("%f\n", seconds);

    free(calc);
    free(threads);
    pthread_mutex_destroy(&mutex);

    if (shm_ptr) {
        munmap(shm_ptr, sizeof(SharedResult));
        shm_unlink(MC_SHM_NAME);
    }

    return 0;
}