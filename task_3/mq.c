#include "common.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define TYPE 1L

typedef struct {
    long type;
    uint32_t len;
    int end;
    char data[];
} Message;

static int send_msg(int id, Message *m, size_t len) {
    while (msgsnd(id, m, len, 0) == -1) {
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

    FILE *f = fopen("/proc/sys/kernel/msgmax", "r");
    unsigned long maxmsg = 8192;
    if (f) { if (fscanf(f, "%lu", &maxmsg) != 1) maxmsg = 8192; fclose(f); }
    if (maxmsg <= sizeof(uint32_t) + sizeof(int)) {
        fprintf(stderr, "kernel message limit is too small\n"); return 1;
    }
    size_t part = (size_t)maxmsg - sizeof(uint32_t) - sizeof(int);
    size_t alloc = cap < part ? cap : part;
    Message *m = malloc(sizeof(Message) + alloc);
    if (!m) { perror("malloc"); return 1; }
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (qid == -1) { perror("msgget"); return 1; }
    m->type = TYPE;

    double start = now_sec();
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) {
        for (;;) {
            ssize_t n;
            do { n = msgrcv(qid, m, sizeof(m->len) + sizeof(m->end) + alloc, TYPE, 0); } while (n == -1 && errno == EINTR);
            if (n == -1) _exit(2);
            if (m->end) break;
            if (write_all(out, m->data, m->len) == -1) _exit(3);
        }
        close(out); close(in); free(m); _exit(0);
    }

    int failed = 0;
    for (;;) {
        int n = read_all(in, m->data, alloc);
        if (n < 0) { perror("read"); failed = 1; break; }
        m->len = (uint32_t)n;
        m->end = (n == 0);
        if (send_msg(qid, m, sizeof(m->len) + sizeof(m->end) + m->len) == -1) {
            perror("msgsnd"); failed = 1; break;
        }
        if (m->end) break;
    }
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) { perror("waitpid"); failed = 1; }
    double elapsed = now_sec() - start;
    printf("%.6f\n", elapsed);
    close(in); close(out); free(m); msgctl(qid, IPC_RMID, NULL);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
    return failed;
}
