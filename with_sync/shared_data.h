#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>

#define SHM_NAME "/mc_shm_sync"

struct SharedData {
    long long global_hits;
    pthread_mutex_t mutex; 
};

#endif

