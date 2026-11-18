#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "4_task.h"

int main() {
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd < 0) {
        perror("shm_open (сначала запустите программу A)");
        return 1;
    }

    shm_data_t* shm = mmap(NULL, sizeof(shm_data_t), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(shm_fd);

    if (!shm->ready) {
        printf("B: программа A ещё не закончила расчёт\n");
        munmap(shm, sizeof(shm_data_t));
        return 1;
    }

    long long total_hits = 0;
    for (int i = 0; i < shm->n_threads; i++) {
        total_hits += shm->hits[i];
    }

    double area = (B_BOUND - A_BOUND) * F_MAX;
    double result = area * (double) total_hits / (double) shm->n_points;
    double exact = (B_BOUND * B_BOUND * B_BOUND - A_BOUND * A_BOUND * A_BOUND) / 3.0;

    printf("B: интеграл = %.6f, точное = %.6f, погрешность = %.6f\n", result, exact, result - exact);
    printf("B: потоков = %d, время расчёта = %.4f с\n", shm->n_threads, shm->time_sec);

    munmap(shm, sizeof(shm_data_t));
    shm_unlink(SHM_NAME);
    return 0;
}
