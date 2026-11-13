#define _POSIX_C_SOURCE 200809L

#include "shared.h"

#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [SHM_NAME]\n", argv[0]);
        return 1;
    }
    const char *name = argc == 2 ? argv[1] : DEFAULT_SHM_NAME;
    int descriptor = shm_open(name, O_RDONLY, 0);
    if (descriptor == -1) {
        perror("shm_open (run program_a first)");
        return 1;
    }
    struct stat info;
    if (fstat(descriptor, &info) == -1 || info.st_size < (off_t)sizeof(SharedResults)) {
        fprintf(stderr, "Shared memory is not ready. Run program_b after program_a.\n");
        close(descriptor);
        return 1;
    }
    size_t bytes = (size_t)info.st_size;
    const SharedResults *shared = mmap(NULL, bytes, PROT_READ, MAP_SHARED, descriptor, 0);
    close(descriptor);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    if (!atomic_load_explicit(&shared->ready, memory_order_acquire)) {
        fprintf(stderr, "Calculation has not finished. Run program_b after program_a.\n");
        munmap((void *)shared, bytes);
        return 1;
    }
    if (shared->threads == 0 || shared->threads > 4096 || shared->points == 0 ||
        bytes != sizeof(*shared) + shared->threads * sizeof(ThreadResult)) {
        fprintf(stderr, "Invalid shared memory data\n");
        munmap((void *)shared, bytes);
        return 1;
    }

    uint64_t total_points = 0;
    uint64_t total_hits = 0;
    for (unsigned int index = 0; index < shared->threads; ++index) {
        total_points += shared->results[index].points;
        total_hits += shared->results[index].hits;
    }
    if (total_points != shared->points || total_hits > total_points) {
        fprintf(stderr, "Incomplete results\n");
        munmap((void *)shared, bytes);
        return 1;
    }
    double integral = (double)total_hits / (double)total_points;
    printf("function=x^2 on [0,1]\n");
    printf("threads=%u\npoints=%" PRIu64 "\nhits=%" PRIu64 "\n",
           shared->threads, total_points, total_hits);
    printf("integral=%.10f\nexact=%.10f\nerror=%.10f\n",
           integral, 1.0 / 3.0, fabs(integral - 1.0 / 3.0));
    printf("seconds=%.9f\nseed=%" PRIu64 "\n", shared->seconds, shared->seed);
    munmap((void *)shared, bytes);
    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
        return 1;
    }
    return 0;
}
