// migration.c  —  gcc -O2 -pthread migration.c -o migration
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

static atomic_int stop = 0;

// вернуть номер CPU, на котором сейчас исполняется вызывающий поток
static int current_cpu(void) {
    return sched_getcpu();
}

// привязать вызывающий поток к набору CPU из строки, напр. "0" или "0,1"
static void pin_to(const char *cpus) {
    cpu_set_t set;
    CPU_ZERO(&set);
    char *dup = strdup(cpus), *tok = strtok(dup, ",");
    while (tok) { CPU_SET(atoi(tok), &set); tok = strtok(NULL, ","); }
    free(dup);
    if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) != 0) {
        perror("setaffinity");
        exit(1);
    }
}

// --- поток 1: жёстко на ядре 1, всё время грузит CPU ---
static void *worker_pinned(void *arg) {
    pin_to("1");
    printf("[P1] pinned to CPU 1, start\n");
    while (!stop) {
        // немного полезной работы, чтобы не оптимизировалось
        volatile double x = 0;
        for (int i = 0; i < 1000000; i++) x += i * 0.5;
    }
    return NULL;
}

// --- поток 2: может бегать по ядрам 1 и 2, логирует миграции ---
static void *worker_migrating(void *arg) {
    pin_to("1,2");
    int last = -1;
    printf("[P2] allowed CPUs = {1,2}, start\n");

    while (!stop) {
        int c = current_cpu();

        // логируем только смену ядра
        if (c != last) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            printf("[P2] t=%ld.%03lds  MIGRATION -> CPU %d\n",
                   ts.tv_sec, ts.tv_nsec / 1000000, c);
            fflush(stdout);
            last = c;
        }
        usleep(2000);   // опрос каждые ~2 мс, чтобы не жечь CPU
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker_pinned,    NULL);
    pthread_create(&t2, NULL, worker_migrating, NULL);

    // Фаза 1: лёгкая нагрузка (только P1 на ядре 1)
    printf("=== Phase 1: low load, P2 should sit on CPU 1 ===\n");
    sleep(3);

    // Фаза 2: резко добавляем нагрузку на ядро 1 —
    // P2 должен мигрировать на CPU 2
    printf("=== Phase 2: heavy load on CPU 1 -> expect migration of P2 ===\n");
    for (int i = 0; i < 3; i++) {
        pthread_t t;
        pthread_create(&t, NULL, worker_pinned, NULL);
        pthread_detach(t);
    }

    sleep(5);
    stop = 1;
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
