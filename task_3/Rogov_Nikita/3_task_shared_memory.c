#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SEM_EMPTY 0
#define SEM_FULL  1

#ifndef __APPLE__
union semun {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};
#endif

typedef struct {
    size_t bytes;
    char data[];
} shm_packet_t;

void sem_change (int semid, int sem_num, int delta) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = delta;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) < 0) {
        perror("semop failed");
        exit(1);
    }
}

int main (int argc, char* argv[]) {
    if (argc < 4) {
        printf("Использование: %s <input> <output> <buf_size_bytes>\n", argv[0]);
        return 1;
    }

    char* input_file = argv[1];
    char* output_file = argv[2];
    size_t buf_size = atoll(argv[3]);

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) {
        perror("open input");
        return 1;
    }

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("open output");
        close(in_fd);
        return 1;
    }

    int semid = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    if (semid < 0) {
        perror("semget");
        return 1;
    }

    union semun arg;
    unsigned short vals[2] = {1, 0}; // SEM_EMPTY = 1, SEM_FULL = 0

    arg.array = vals;
    if (semctl(semid, 0, SETALL, arg) < 0) {
        perror("semctl SETALL");
        return 1;
    }

    size_t shm_size = sizeof(shm_packet_t) + buf_size;
    int shmid = shmget(IPC_PRIVATE, shm_size, 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget");
        printf("buf_size превышает системный лимит\n");
        return 1;
    }

    shm_packet_t* packet = (shm_packet_t*) shmat(shmid, NULL, 0);
    if (packet == (void*)-1) {
        perror("shmat");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { //child process
        close(in_fd);

        while (1) {
            sem_change(semid, SEM_FULL, -1);

            if (packet->bytes == 0) {
                break;
            }

            write(out_fd, packet->data, packet->bytes);

            sem_change(semid, SEM_EMPTY, 1);
        }

        close(out_fd);
        shmdt(packet);
        exit(0);
    }
    else { //parent process
        close(out_fd);

        while (1) {
            sem_change(semid, SEM_EMPTY, -1);

            ssize_t bytes_read = read(in_fd, packet->data, buf_size);
            packet->bytes = bytes_read;

            sem_change(semid, SEM_FULL, 1);

            if (bytes_read == 0) {
                break;
            }
        }

        close(in_fd);
        wait(NULL);

        shmdt(packet);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
    }

    return 0;
}
