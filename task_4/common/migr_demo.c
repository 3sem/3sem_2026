#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


static void pin(const char *m) {
    cpu_set_t s; CPU_ZERO(&s);
    for (char *p = strtok(strdup(m), ","); p; p = strtok(NULL, ","))
        CPU_SET(atoi(p), &s);
    pthread_setaffinity_np(pthread_self(), sizeof(s), &s);
}

static void *hog(void *_) { pin("0"); for (;;) ; }        // всегда на CPU 0
static void *watcher(void *_) {                            // может на 0 или 1
    pin("0,1");
    int last = -1;
    for (;;) {
        int c = sched_getcpu();
        if (c != last) printf("CPU %d\n", last = c), fflush(stdout);
        usleep(1000);
    }
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, watcher, NULL);
    pthread_create(&b, NULL, hog, NULL);      // 1 поток на CPU 0
    sleep(2);
    pthread_create(&b, NULL, hog, NULL);      // +3 потока → перегруз CPU 0
    pthread_create(&b, NULL, hog, NULL);
    pthread_create(&b, NULL, hog, NULL);
    pause();
}
