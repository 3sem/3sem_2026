#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "common.h"

int main() {
    int shmid = shmget(SHM_KEY, sizeof(shm_data_t), 0666);
    if (shmid == -1) {
        perror("shmget (Run ProgA first!)");
        return 1;
    }

    shm_data_t *shm_ptr = (shm_data_t*)shmat(shmid, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("shmat");
        return 1;
    }

    double total_integral = 0.0;
    int n = shm_ptr->num_threads;

    for (int i = 0; i < n; i++) {
        total_integral += shm_ptr->results[i];
    }
    total_integral /= n; 

    printf("[ProgB] Calculated Integral: %.6f (Analytical: 0.333333)\n", total_integral);

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL); 
    return 0;
}