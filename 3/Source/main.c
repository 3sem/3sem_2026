#if defined(SYS_V_QUEUE)
#include "sys_v_queue.h"

#elif defined(FIFO)
#include "FIFO.h"

#elif defined(SHM)
#include "shm.h"
#endif

#include <time.h>

int main() {
    pid_t pid;
    int status;
    struct timespec start, end;

#if defined(SYS_V_QUEUE)
    printf("============SYS V QUEUE check===========\n");

    buffer_t *buffer = create_buffer();
    if (!buffer) return 0;

    sys_v_queue_t *sys_v_queue  = create_sys_v_queue();
    if (!sys_v_queue) return 0;

#elif defined(FIFO)
    printf("============FIFO check===========\n");

    ch_t *ch = create_channel(default_fifo_path);
    if (!ch) return 0;

    buffer_t *buffer = create_buffer();
    if (!buffer) return 0;
#elif defined(SHM)
    printf("============SHM check==========\n");

    sem_t *sem = create_sem();
    if (!sem) return 0;

    shm_t *shm = creat_shm();
    if (!shm) return 0;
#endif

    clock_gettime(CLOCK_MONOTONIC, &start);
    pid = fork();

    switch (pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0:
            fprintf(stderr, "child start\n");
#if defined(SYS_V_QUEUE)
            start_child(sys_v_queue, buffer);
#elif defined(FIFO)
            start_child(ch, buffer);
#elif defined(SHM)
            start_child(shm, sem);
#endif
            fprintf(stderr, "child end\n");
            break;

        default:
            fprintf(stderr, "parent start\n");
#if defined(SYS_V_QUEUE)
            start_parent(sys_v_queue, buffer);
#elif defined(FIFO)
            start_parent(ch, buffer);
#elif defined(SHM)
            start_parent(shm, sem);
#endif
            waitpid(pid, &status, 0);
            fprintf(stderr, "parent end\n");
            printf("===============================\n");
            break;
    }



    if (pid != 0){
        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed_time = ((double)end.tv_sec - (double)start.tv_sec) + 
                      ((double)end.tv_nsec - (double)start.tv_nsec) / 1000000000.0;

        printf("PARENT REAL TIME: %.5lf seconds\n", elapsed_time);
    }

#if defined(SYS_V_QUEUE)
    destroy_sys_v_queue(sys_v_queue);
    destroy_buffer(buffer);
#elif defined(FIFO)
    destroy_channel(ch);
    destroy_buffer(buffer);
#elif defined(SHM)
    if (pid) destroy_shm(shm);
    if (pid) destroy_sem(sem);
#endif
}