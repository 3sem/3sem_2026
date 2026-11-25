#include "common.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define EMPTY 0
#define FULL 1
#define STOP 2

typedef struct {
    size_t len;
    int end;
    char data[];
} Shared;

union semun { int val; unsigned short *array; struct semid_ds *buf; };

static int sem_op(int id, unsigned short num, short op) {
    struct sembuf b = {num, op, 0};
    while (semop(id, &b, 1) == -1) {
        if (errno != EINTR) return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s input output buffer_size\n", argv[0]);
        return 2;
    }
    size_t cap = get_size(argv[3]);
    int in = open_input(argv[1]);
    if (in == -1) return 1;
    int out = open_output(argv[2]);
    if (out == -1) { close(in); return 1; }

    int shmid = shmget(IPC_PRIVATE, sizeof(Shared) + cap, IPC_CREAT | 0600);
    int semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (shmid == -1 || semid == -1) { perror("shmget/semget"); return 1; }
    Shared *sh = shmat(shmid, NULL, 0);
    if (sh == (void *)-1) { perror("shmat"); return 1; }
    unsigned short vals[2] = {1, 0};
    union semun arg; arg.array = vals;
    if (semctl(semid, 0, SETALL, arg) == -1) { perror("semctl"); return 1; }

    double start = now_sec();
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) {
        for (;;) {
            if (sem_op(semid, FULL, -1) == -1) _exit(2);
            if (sh->end) break;
            if (write_all(out, sh->data, sh->len) == -1) _exit(3);
            if (sem_op(semid, EMPTY, 1) == -1) _exit(4);
        }
        close(out);
        close(in);
        shmdt(sh);
        _exit(0);
    }

    int failed = 0;
    for (;;) {
        if (sem_op(semid, EMPTY, -1) == -1) { failed = 1; break; }
        int n = read_all(in, sh->data, cap);
        if (n < 0) { perror("read"); failed = 1; sh->end = 1; sh->len = 0; }
        else { sh->len = (size_t)n; sh->end = (n == 0); }
        if (sem_op(semid, FULL, 1) == -1) { failed = 1; break; }
        if (sh->end) break;
    }
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) { perror("waitpid"); failed = 1; }
    double elapsed = now_sec() - start;
    printf("%.6f\n", elapsed);
    close(in); close(out); shmdt(sh);
    shmctl(shmid, IPC_RMID, NULL); semctl(semid, 0, IPC_RMID);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
    return failed;
}
