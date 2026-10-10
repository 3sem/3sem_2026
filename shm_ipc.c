// shm_ipc.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// Структуры для работы с семафорами
struct sembuf sem_lock = {0, -1, 0};
struct sembuf sem_unlock = {0, 1, 0};

void sem_op(int semid, int sem_num, int op) {
    struct sb;
    struct sembuf s = {sem_num, op, 0};
    semop(semid, &s, 1);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <file_in> <file_out> <buf_size>\n", argv[0]);
        return 1;
    }
    char *file_in = argv[1];
    char *file_out = argv[2];
    size_t buf_size = atoi(argv[3]);

    // Создаем SHM и семафоры
    int shmid = shmget(SHM_KEY, buf_size + sizeof(size_t), IPC_CREAT | 0666);
    int semid = semget(SEM_KEY, 2, IPC_CREAT | 0666); // 0: empty (writer), 1: full (reader)
    
    semctl(semid, 0, SETVAL, 1); // Писатель может начинать
    semctl(semid, 1, SETVAL, 0); // Читателю ждать

    pid_t pid = fork();
    if (pid == 0) { // Читатель (Потомок) -> Пишет в файл
        void *shm_ptr = shmat(shmid, NULL, 0);
        size_t *bytes_in_shm = (size_t*)shm_ptr;
        char *data_ptr = (char*)shm_ptr + sizeof(size_t);
        FILE *out = fopen(file_out, "wb");

        while (1) {
            sem_op(semid, 1, -1); // Ждем данные от писателя
            if (*bytes_in_shm == 0) break; // Конец файла
            fwrite(data_ptr, 1, *bytes_in_shm, out);
            sem_op(semid, 0, 1); // Освобождаем буфер для писателя
        }
        fclose(out);
        shmdt(shm_ptr);
        return 0;
    } else { // Писатель (Родитель) -> Читает из файла
        void *shm_ptr = shmat(shmid, NULL, 0);
        size_t *bytes_in_shm = (size_t*)shm_ptr;
        char *data_ptr = (char*)shm_ptr + sizeof(size_t);
        FILE *in = fopen(file_in, "rb");

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        while (1) {
            sem_op(semid, 0, -1); // Ждем пустой буфер
            size_t read_bytes = fread(data_ptr, 1, buf_size, in);
            *bytes_in_shm = read_bytes;
            sem_op(semid, 1, 1); // Уведомляем читателя
            if (read_bytes == 0) break;
        }
        
        fclose(in);
        shmdt(shm_ptr);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("SHM Time (%zu bytes): %.4f sec\n", buf_size, elapsed);

        // Очистка IPC
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
    }
    return 0;
}
