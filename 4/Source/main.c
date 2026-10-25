
#include "include.h"
#include <locale.h>

int read_results(sem_t *sem, shm_t *shm);

int main() {
    setlocale(LC_NUMERIC, "ru_RU.UTF-8"); // , в вывод вместо .

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid;
    int   status;
    sem_t *sem;
    shm_t *shm;

    if (!(sem = creat_sem())) return 0;
    if (!(shm = creat_shm())) return 0;

    pid = fork();

    switch (pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0: 
            create_pthreads(sem, shm);
            exit(EXIT_SUCCESS); 
            break;

        default:
            read_results(sem, shm);
            waitpid(pid, &status, 0);
            break;
    }

    destroy_sem(sem);
    destroy_shm(shm);

    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double time_taken = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("time: %.4f sec\n", time_taken);

    return 0;
}

int read_results(sem_t *sem, shm_t *shm){
    int num = 0;
    sem->sem_ops->wait_reader(sem);


    for (int test_num = 0; test_num < PTHREADS_NUM; test_num++){
        num += shm->buffer->buffer[test_num];
    }

    float S = (float)num / (PTHREADS_NUM * POINTS_IN_PTHREAD) *\
                (default_points_param.xmax - default_points_param.xmin) * \
                (default_points_param.ymax - default_points_param.ymin);
    printf("INT: %f\n", S);
    return 0;
}

int create_pthreads(sem_t *sem, shm_t *shm){
    for (int pthread_id = 0; pthread_id < PTHREADS_NUM; pthread_id++){
        thread_args_t *args = malloc(sizeof(thread_args_t));

        args->points        = default_points_param; 
        args->points.num    = POINTS_IN_PTHREAD;
        args->user_func     = int_func;
        args->sem           = sem;
        args->shm           = shm;
        args->pthread_id    = pthread_id;
        
        if (pthread_create(&pthreads[pthread_id], NULL, check_points_set, args) != 0) {
            perror("pthread_create failed");
            free(args);
            return -1;
        }
    }

    for (int pthreads_id = 0; pthreads_id < PTHREADS_NUM; pthreads_id++){
        if (pthread_join(pthreads[pthreads_id], NULL) != 0) {
            perror("pthread_join failed");
        }
    }
    sem->sem_ops->open_reader(sem);


    return 0;
}

void *check_points_set(void *thread_args){
    thread_args_t *args = (thread_args_t *)(thread_args);

    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    CPU_SET(args->pthread_id % max_cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    int num;
    if ((num = check_points(args->points, args->user_func, args->pthread_id)) == -1) return 0;

    args->shm->buffer->actions->put_num_in_buf(args->shm->buffer, num, args->pthread_id);

    printf("%d: \t%d\n", args->pthread_id, num);

    free(args);

    return NULL;
}