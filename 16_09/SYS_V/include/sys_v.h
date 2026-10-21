#ifndef SYS_V_H
#define SYS_V_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

#define IPC_KEY_FILE "ipc.key"

enum IpcProjectId {
    SHM_PROJECT_ID = 'M',
    SEM_PROJECT_ID = 'S',
    QUEUE_PROJECT_ID = 'Q'
};

enum SemaphoreIndex {
    SEM_FULL = 0,
    SEM_EMPTY = 1,
    SEM_MUTEX = 2,
    SEM_COUNT = 3
};

struct SharedData {
    size_t capacity;
    size_t size;
    bool finished;
    bool failed;
    char buff[];
};

#endif
