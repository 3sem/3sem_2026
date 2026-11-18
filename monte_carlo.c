#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>

#define SHM_NAME "/monte_carlo_threads_shm"

#define LIMIT_A 0.0
#define LIMIT_B 1.0

typedef struct {
    int total_threads;
    unsigned long long N; 
    
    double total_integral;

    pthread_mutex_t mutex;
} SharedData;

typedef struct {
    int thread_id;
    SharedData* shm_data;
} ThreadArgs;

double f(double x) {
    return x * x;
}

// Функция для потока-счеткика
void* monte_carlo_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    SharedData* data = args->shm_data;
    int id = args->thread_id;
    int n = data->total_threads;

    double segment_width = (LIMIT_B - LIMIT_A) / n;
    double x_min = LIMIT_A + id * segment_width;
    double x_max = x_min + segment_width;

    double y_min = 0.0;
    double y_max = f(x_max);//считаю, что функция монотонная 

    double local_rectangle_area = segment_width * (y_max - y_min);

    unsigned int seed = time(NULL) ^ pthread_self() ^ id;
    unsigned long long hits = 0;

    struct timespec start_cpu, end_cpu;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_cpu);

    for (unsigned long long i = 0; i < data->N / data -> total_threads; i++) {
        double x = x_min + ((double)rand_r(&seed) / RAND_MAX) * (x_max - x_min);
        double y = y_min + ((double)rand_r(&seed) / RAND_MAX) * (y_max - y_min);

        if (y <= f(x)) {
            hits++;
        }
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_cpu);
    double cpu_time = (end_cpu.tv_sec - start_cpu.tv_sec) + 
                      (end_cpu.tv_nsec - start_cpu.tv_nsec) / 1000000000.0;

    printf("[Thread %d]CPU time: %.4f sec\n", id, cpu_time);

    double local_integral = ((double)hits / (data->N / data->total_threads)) * local_rectangle_area;

    pthread_mutex_lock(&data->mutex);
    data->total_integral += local_integral;
    pthread_mutex_unlock(&data->mutex);

    return NULL;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <threads_count> <points>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    unsigned long long total_points = strtoull(argv[2], NULL, 10);

    if (n <= 0) n = 1;

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (shm_fd < 0){
         perror("shm_open");
        return 1; 
    }
    
    if (ftruncate(shm_fd, sizeof(SharedData)) < 0){ 
        perror("ftruncate"); 
        return 1; 
    }

    SharedData* data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED){ 
        perror("mmap");
        return 1; 
    }

    data->total_threads = n;
    data->N = total_points;
    data->total_integral = 0.0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_t* threads = malloc(sizeof(pthread_t) * n);
    ThreadArgs* t_args = malloc(sizeof(ThreadArgs) * n);

    struct timespec start_real, end_real;
    clock_gettime(CLOCK_MONOTONIC, &start_real);

    for (int i = 0; i < n; i++) {
        t_args[i].thread_id = i;
        t_args[i].shm_data = data;
        pthread_create(&threads[i], NULL, monte_carlo_thread, &t_args[i]);
    }

    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_real);
    double real_time = (end_real.tv_sec - start_real.tv_sec) + 
                        (end_real.tv_nsec - start_real.tv_nsec) / 1000000000.0;

    printf("[Main Thread] All threads finished.\n");

    double final_integral = data->total_integral;
    
    printf("\n==============================================\n");
    printf("Total Real Time:               %.4f sec.\n", real_time);
    printf("Threads count:                 %d\n", data->total_threads);
    printf("Total points generated:        %llu\n", (unsigned long long)data->N);
    printf("Integral result:               %f\n", final_integral);
    printf("Exact value (x^2):             0.333333\n");
    printf("=======================================================\n");

    pthread_mutex_destroy(&data->mutex);
    free(threads);
    free(t_args);
    munmap(data, sizeof(SharedData));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}
