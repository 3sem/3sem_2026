#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#define SHM_NAME "/mc_shm_nosync"
#define MAX_THREADS 128

struct SharedData {
    // Независимые ячейки для каждого потока
    long long local_hits[MAX_THREADS];
};

#endif
