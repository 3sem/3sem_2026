#include "sys_v.h"
#include "ipc_utils.h"

#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>

static struct SharedData *openSharedMemory(key_t shm_key) {
    int shmid = shmget(shm_key, sizeof(struct SharedData), 0);
    if (shmid == -1) {
        perror("shmget");
        return NULL;
    }

    struct shmid_ds shm_info;
    if (shmctl(shmid, IPC_STAT, &shm_info) == -1) {
        perror("shmctl IPC_STAT");
        return NULL;
    }

    if (shm_info.shm_segsz < sizeof(struct SharedData)) {
        fprintf(stderr, "Invalid shared memory size\n");
        return NULL;
    }

    void *memory = shmat(shmid, NULL, 0);
    if (memory == (void *)-1) {
        perror("shmat");
        return NULL;
    }

    struct SharedData *data = memory;

    if (data->capacity == 0 ||
        data->capacity > shm_info.shm_segsz - sizeof(*data)) {
        fprintf(stderr, "Invalid shared memory capacity\n");
        shmdt(data);
        return NULL;
    }

    return data;
}

static int readMessage(const struct SharedData *data, FILE *output) {
    if (data->size > data->capacity) {
        fprintf(stderr, "Invalid message size\n");
        return -1;
    }

    if (data->size > 0 &&
        fwrite(data->buff, 1, data->size, output) != data->size) {
        perror("fwrite");
        return -1;
    }

    return 0;
}

static int receiveFile(struct SharedData *data, FILE *output, int semid) {
    while (true) {
        if (waitSemaphore(semid, SEM_FULL) == -1) {
            return -1;
        }

        if (waitSemaphore(semid, SEM_MUTEX) == -1) {
            postSemaphore(semid, SEM_EMPTY);
            return -1;
        }

        bool finished = data->finished;
        int result = readMessage(data, output);

        if (result == -1) {
            data->failed = true;
        }

        if (postSemaphore(semid, SEM_MUTEX) == -1) {
            postSemaphore(semid, SEM_EMPTY);
            return -1;
        }

        if (postSemaphore(semid, SEM_EMPTY) == -1) {
            return -1;
        }

        if (result == -1) {
            return -1;
        }

        if (finished) {
            return 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    FILE *output = fopen(argv[1], "wb");
    if (output == NULL) {
        perror("fopen output");
        return 1;
    }

    key_t shm_key = getIpcKey(IPC_KEY_FILE, SHM_PROJECT_ID);
    key_t sem_key = getIpcKey(IPC_KEY_FILE, SEM_PROJECT_ID);

    if (shm_key == (key_t)-1 || sem_key == (key_t)-1) {
        fclose(output);
        return 1;
    }

    struct SharedData *data = openSharedMemory(shm_key);
    if (data == NULL) {
        fclose(output);
        return 1;
    }

    int semid = semget(sem_key, SEM_COUNT, 0);
    if (semid == -1) {
        perror("semget");
        detachSharedMemory(data);
        fclose(output);
        return 1;
    }

    int result = receiveFile(data, output, semid);

    if (fclose(output) == EOF) {
        perror("fclose output");
        result = -1;
    }

    if (detachSharedMemory(data) == -1) {
        result = -1;
    }

    return result == 0 ? 0 : 1;
}

