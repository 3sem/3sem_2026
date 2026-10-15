#define _GNU_SOURCE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "monte_carlo.h"

int main(void) {
    key_t key = ftok(SHM_KEY_PATH, SHM_KEY_ID);
    if (key == -1) {
        perror("ftok");
        return 1;
    }

    int shmid = shmget(key, sizeof(struct SharedResults), 0666);
    if (shmid == -1) {
        perror("shmget (программа А ещё не запускалась?)");
        return 1;
    }

    struct SharedResults *shared = (struct SharedResults*)shmat(shmid, NULL, 0);
    if (shared == (void*)-1) {
        perror("shmat");
        return 1;
    }

    printf("=== Программа Б: читаем из shm (shmid=%d) ===\n", shmid);
    printf("num_threads: %d\n", shared->num_threads);
    printf("mode:        %s\n", shared->mode == MODE_INDEPENDENT ? "INDEPENDENT" : "SHARED");
    printf("total_points: %zu\n", shared->total_points);
    printf("points_per_thread: %zu\n", shared->points_per_thread);
    printf("result:      %.10f\n", shared->result);

    if (shared->mode == MODE_INDEPENDENT) {
        printf("Счётчики по потокам:\n");
        size_t total = 0;
        for (int i = 0; i < shared->num_threads; i++) {
            printf("  поток %d: %zu\n", i, shared->counters[i]);
            total += shared->counters[i];
        }
        printf("Сумма: %zu\n", total);
    } else {
        printf("Общий счётчик: %zu\n", shared->counters[0]);
    }

shmdt(shared);
shmctl(shmid, IPC_RMID, NULL);   /* удалить shm */
printf("shm удалён\n");
return 0;
}