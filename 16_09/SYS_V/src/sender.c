#include "sys_v.h"
#include "ipc_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/sem.h>
#include <stdlib.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

static int prepareSharedMemory(key_t shm_key, int *shmid, size_t buffer_size,
                               struct SharedData **data) {
    if (buffer_size == 0 || buffer_size > SIZE_MAX - sizeof(struct SharedData)) {
        fprintf(stderr,"buff_size\n");
        return -1;
    }

    *shmid = shmget(shm_key, sizeof(struct SharedData) + buffer_size,
                    IPC_CREAT | IPC_EXCL | 0600);
    if (*shmid == -1) {
        perror("shmget");
        return -1;
    }

    void *memory = shmat(*shmid, NULL, 0);
    if (memory == (void *)-1) {
        perror("shmat");
        return -1;
    }

    *data = memory;
    (*data)->capacity = buffer_size;
    return 0;
}

static int prepareSemaphoreSet(key_t sem_key, int *semid) {
    *semid = semget(sem_key, SEM_COUNT, IPC_CREAT | IPC_EXCL | 0600);
    if (*semid == -1) {
        perror("semget");
        return -1;
    }

    unsigned short values[SEM_COUNT] = {
        [SEM_FULL] = 0,
        [SEM_EMPTY] = 1,
        [SEM_MUTEX] = 1
    };

    union semun argument = {
        .array = values
    };

    if (semctl(*semid, 0, SETALL, argument) == -1) {
        perror("semctl SETALL");
        return -1;
    }

    return 0;
}

static int writeMessage(struct SharedData *data, FILE *input) {
    size_t bytes_read = fread(data->buff, 1, data->capacity, input);

    if (ferror(input)) {
        perror("fread");
        return -1;
    }

    data->size = bytes_read;
    data->finished = feof(input);

    return 0;
}

static int sendFile(struct SharedData *data, FILE *input, int semid) {
    if (waitSemaphore(semid, SEM_EMPTY) == -1) {
        return -1;
    }

    while (true) {
        if (waitSemaphore(semid, SEM_MUTEX) == -1) {
            return -1;
        }

        if (data->failed) {
            postSemaphore(semid, SEM_MUTEX);
            fprintf(stderr, "Receiver reported a write error\n");
            return -1;
        }

        if (writeMessage(data, input) == -1) {
            postSemaphore(semid, SEM_MUTEX);
            return -1;
        }

        bool finished = data->finished;

        if (postSemaphore(semid, SEM_MUTEX) == -1) {
            return -1;
        }

        if (postSemaphore(semid, SEM_FULL) == -1) {
            return -1;
        }

        if (waitSemaphore(semid, SEM_EMPTY) == -1) {
            return -1;
        }

        if (waitSemaphore(semid, SEM_MUTEX) == -1) {
            return -1;
        }

        bool failed = data->failed;

        if (postSemaphore(semid, SEM_MUTEX) == -1) {
            return -1;
        }

        if (failed) {
            fprintf(stderr, "Receiver reported a write error\n");
            return -1;
        }

        if (finished) {
            return 0;
        }
    }
}

static void cleanup(struct SharedData *data, int shmid, int semid) {
    detachSharedMemory(data);
    removeSemaphoreSet(semid);
    removeSharedMemory(shmid);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <buffer_size>\n", argv[0]);
        return 1;
    }

    size_t buffer_size = strtoul(argv[2], NULL, 10);

    if (buffer_size == 0) {
        fprintf(stderr, "Buffer size must be positive\n");
        return 1;
    }
    

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("fopen input");
        return 1;
    }

    key_t shm_key = getIpcKey(IPC_KEY_FILE, SHM_PROJECT_ID);
    key_t sem_key = getIpcKey(IPC_KEY_FILE, SEM_PROJECT_ID);

    if (shm_key == (key_t)-1 || sem_key == (key_t)-1) {
        fclose(input);
        return 1;
    }

    int shmid = -1;
    int semid = -1;
    struct SharedData *data = NULL;

    if (prepareSharedMemory(shm_key, &shmid, buffer_size, &data) == -1) {
        fclose(input);
        cleanup(data, shmid, semid);
        return 1;
    }

    data->size = 0;
    data->finished = false;
    data->failed = false;

    if (prepareSemaphoreSet(sem_key, &semid) == -1) {
        fclose(input);
        cleanup(data, shmid, semid);
        return 1;
    }

    int result = sendFile(data, input, semid);

    if (fclose(input) == EOF) {
        perror("fclose input");
        result = -1;
    }

    cleanup(data, shmid, semid);

    return result == 0 ? 0 : 1;
}
