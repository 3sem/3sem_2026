// помним, что обработчики ставятся попроцессно, блокировки -- попоточно
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static pthread_t thr;

static void handler(int sig) {
    printf("SIGUSR1 in thread %lu\n", (unsigned long)pthread_self());
}

static void *worker(void *_) {
    sigset_t s; sigemptyset(&s); sigaddset(&s, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &s, NULL);   // разблокировали в этом потоке
    pause();                                   // ждём сигнал
    return NULL;
}

int main(void) {
    signal(SIGUSR1, handler);                  // диспозиция — на процесс

    sigset_t s; sigemptyset(&s); sigaddset(&s, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &s, NULL);      // в main блокируем, чтобы не поймал сам

    pthread_create(&thr, NULL, worker, NULL);
    sleep(1);

    pthread_kill(thr, SIGUSR1);                // адресно в worker
    pthread_join(thr, NULL);
    return 0;
}
