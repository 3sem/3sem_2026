#define _XOPEN_SOURCE 500 
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>      
#include "shared_data.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <N_всего> <n_потоков>\n", argv[0]);
        return 1;
    }

    long long N = atoll(argv[1]);
    int n = atoi(argv[2]);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct SharedData));
    struct SharedData* shm_ptr = (struct SharedData*)mmap(0, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    for (int i = 0; i < MAX_THREADS; ++i) {
        shm_ptr->local_hits[i] = 0;
    }

    printf("[Б без синхронизации]: SHM готова. Запустите Программу А, затем нажмите ENTER...\n");
    getchar();

    long long total_hits = 0;
    for (int i = 0; i < n; ++i) {
        total_hits += shm_ptr->local_hits[i];
    }

    double S_box = 16.0;
    long long actual_N = (N / n) * n;
    double integral = S_box * ((double)total_hits / (double)actual_N);

    printf("--- РЕЗУЛЬТАТЫ (БЕЗ СИНХРОНИЗАЦИИ) ---\n");
    printf("Интеграл: %f (Точно: 4.0)\n", integral);

    munmap(shm_ptr, sizeof(struct SharedData));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    return 0;
}
