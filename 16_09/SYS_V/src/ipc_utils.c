#include "ipc_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>

key_t getIpcKey(const char *path, int project_id) {
    int fd = open(path, O_CREAT | O_RDWR, 0600);

    if (fd == -1) {
        perror("open IPC key file");
        return (key_t)-1;
    }

    if (close(fd) == -1) {
        perror("close IPC key file");
        return (key_t)-1;
    }

    key_t key = ftok(path, project_id);

    if (key == (key_t)-1) {
        perror("ftok");
    }

    return key;
}

int waitSemaphore(int semid, unsigned short sem_num) {
    struct sembuf operation = {
        .sem_num = sem_num,
        .sem_op = -1,
        .sem_flg = 0
    };

    int result;

    do {
        result = semop(semid, &operation, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("semop wait");
    }

    return result;
}

int postSemaphore(int semid, unsigned short sem_num) {
    struct sembuf operation = {
        .sem_num = sem_num,
        .sem_op = 1,
        .sem_flg = 0
    };

    int result;

    do {
        result = semop(semid, &operation, 1);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("semop post");
    }

    return result;
}

int detachSharedMemory(void *data) {
    if (data == NULL || data == (void *)-1) {
        return 0;
    }

    if (shmdt(data) == -1) {
        perror("shmdt");
        return -1;
    }

    return 0;
}

int removeSharedMemory(int shmid) {
    if (shmid < 0) {
        return 0;
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
        return -1;
    }

    return 0;
}

int removeSemaphoreSet(int semid) {
    if (semid < 0) {
        return 0;
    }

    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return -1;
    }

    return 0;
}

int removeMessageQueue(int msqid) {
    if (msqid < 0) {
        return 0;
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        if (errno == EIDRM || errno == EINVAL) {
            return 0;
        }

        perror("msgctl IPC_RMID");
        return -1;
    }

    return 0;
}
