#include "monte_carlo_common.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>

typedef struct {
    uint64_t total_points;
    uint64_t thread_count;
    uint64_t hits;
    mc_mode_t mode;
} mc_result_t;

static int attachSharedMemory(
    key_t key,
    int *shmid_out,
    size_t *segment_size_out,
    mc_shared_data_t **shared_out
) {
    int shmid = shmget(key, 1, 0600);
    if (shmid == -1) {
        perror("shmget");
        return -1;
    }

    struct shmid_ds info;
    if (shmctl(shmid, IPC_STAT, &info) == -1) {
        perror("shmctl IPC_STAT");
        return -1;
    }

    size_t data_offset = offsetof(mc_shared_data_t, thread_hits);

    if (info.shm_segsz < data_offset) {
        fprintf(stderr, "Shared-memory segment is too small.\n");
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl IPC_RMID");
        }
        return -1;
    }

    void *memory = shmat(shmid, NULL, SHM_RDONLY);
    if (memory == (void *)-1) {
        perror("shmat");
        return -1;
    }

    *shmid_out = shmid;
    *segment_size_out = info.shm_segsz;
    *shared_out = memory;

    return 0;
}

static int collectResult(
    const mc_shared_data_t *shared,
    size_t segment_size,
    mc_result_t *result
) {
    size_t data_offset = offsetof(mc_shared_data_t, thread_hits);

    if (shared->ready != 1 ||
        shared->total_points == 0 ||
        shared->thread_count == 0 ||
        (shared->mode != MC_MODE_PER_THREAD &&
         shared->mode != MC_MODE_SHARED_MUTEX)) {
        fprintf(stderr, "Shared-memory data is invalid or not ready.\n");
        return -1;
    }

    if (shared->thread_count > (uint64_t)SIZE_MAX ||
        shared->thread_count >
            (SIZE_MAX - data_offset) / sizeof(uint64_t) ||
        data_offset +
            (size_t)shared->thread_count * sizeof(uint64_t) >
            segment_size) {
        fprintf(
            stderr,
            "Shared-memory segment does not contain all thread results.\n"
        );
        return -1;
    }

    result->total_points = shared->total_points;
    result->thread_count = shared->thread_count;
    result->mode = (mc_mode_t)shared->mode;
    result->hits = 0;

    if (result->mode == MC_MODE_SHARED_MUTEX) {
        result->hits = shared->total_hits;
    } else {
        for (uint64_t i = 0; i < result->thread_count; ++i) {
            uint64_t thread_hits = shared->thread_hits[i];

            if (thread_hits > result->total_points - result->hits) {
                fprintf(stderr, "Invalid hit count.\n");
                return -1;
            }

            result->hits += thread_hits;
        }
    }

    if (result->hits > result->total_points) {
        fprintf(stderr, "Invalid hit count.\n");
        return -1;
    }

    return 0;
}

static int releaseSharedMemory(
    mc_shared_data_t *shared,
    int shmid
) {
    int result = 0;

    if (shmdt(shared) == -1) {
        perror("shmdt");
        result = -1;
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        result = -1;
    }

    return result;
}

static void printResult(const mc_result_t *result) {
    double estimate =
        (double)result->hits / (double)result->total_points;

    double absolute_error =
        estimate > 0.5 ? estimate - 0.5 : 0.5 - estimate;

    printf(
        "RESULT,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64
        ",%.12f,%.12g\n",
        modeName(result->mode),
        result->thread_count,
        result->total_points,
        result->hits,
        estimate,
        absolute_error
    );
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--cleanup") == 0) {
        key_t key = getIpcKey();
        if (key == (key_t)-1) {
            return 1;
        }

        return removeSharedMemoryByKey(key) == 0 ? 0 : 1;
    }

    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--cleanup]\n", argv[0]);
        return 1;
    }

    key_t key = getIpcKey();
    if (key == (key_t)-1) {
        return 1;
    }

    int shmid;
    size_t segment_size;
    mc_shared_data_t *shared;

    if (attachSharedMemory(
            key,
            &shmid,
            &segment_size,
            &shared
        ) == -1) {
        return 1;
    }

    mc_result_t result;
    int read_result = collectResult(shared, segment_size, &result);
    int cleanup_result = releaseSharedMemory(shared, shmid);

    if (read_result == -1 || cleanup_result == -1) {
        return 1;
    }

    printResult(&result);
    return 0;
}
