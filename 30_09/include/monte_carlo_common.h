#ifndef MONTE_CARLO_COMMON_H
#define MONTE_CARLO_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/ipc.h>

#define MC_KEY_FILE "monte_carlo.key"
#define MC_PROJECT_ID 'M'

typedef enum {
    MC_MODE_PER_THREAD = 1,
    MC_MODE_SHARED_MUTEX = 2
} mc_mode_t;

typedef struct {
    uint64_t total_points;
    uint64_t thread_count;
    uint64_t total_hits;
    uint32_t mode;
    uint32_t ready;
    pthread_mutex_t mutex;
    uint64_t thread_hits[];
} mc_shared_data_t;

int parseUint64(const char *text, uint64_t *value);
int parseMode(const char *text, mc_mode_t *mode);
const char *modeName(mc_mode_t mode);
key_t getIpcKey(void);
int removeSharedMemoryByKey(key_t key);

#endif
