#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun {
    int val;
};

static void sem_wait(int semid, int num) {
    struct sembuf op = { num, -1, 0 };
    semop(semid, &op, 1);
}

static void sem_post(int semid, int num) {
    struct sembuf op = { num, 1, 0 };
    semop(semid, &op, 1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s input output [bufsize]\n", argv[0]);
        return 1;
    }

    int buf_size = (argc >= 4) ? atoi(argv[3]) : 4096;
    if (buf_size <= 0)
        buf_size = 4096;

    int shmid = shmget(IPC_PRIVATE, sizeof(int) + buf_size,
                       0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    int *shm_size = (int *)shmat(shmid, NULL, 0);
    if (shm_size == (int *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    char *shm_buf = (char *)(shm_size + 1);

    int semid = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        shmdt(shm_size);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);
    arg.val = 0;
    semctl(semid, 1, SETVAL, arg);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1) {
            perror("open");
            return 1;
        }

        while (1) {
            sem_wait(semid, 1);
            int n = *shm_size;
            if (n == 0) {
                sem_post(semid, 0);
                break;
            }
            write(out_fd, shm_buf, n);
            sem_post(semid, 0);
        }

        close(out_fd);
        shmdt(shm_size);
        return 0;
    }

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return 1;
    }

    while (1) {
        sem_wait(semid, 0);
        int n = read(in_fd, shm_buf, buf_size);
        if (n <= 0) {
            *shm_size = 0;
            sem_post(semid, 1);
            break;
        }
        *shm_size = n;
        sem_post(semid, 1);
    }

    close(in_fd);
    wait(NULL);

    shmdt(shm_size);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec)
                   + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    fprintf(stderr, "buf=%d time=%.3f s\n", buf_size, elapsed);

    return 0;
}