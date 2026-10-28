#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MSGMAX 8192

struct msgbuf {
    long mtype;
    char mtext[MSGMAX];
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s input output [bufsize]\n", argv[0]);
        return 1;
    }

    int buf_size = (argc >= 4) ? atoi(argv[3]) : MSGMAX;
    if (buf_size <= 0)
        buf_size = MSGMAX;

    int msqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget");
        return 1;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        msgctl(msqid, IPC_RMID, NULL);
        return 1;
    }

    if (pid == 0) {
        int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1) {
            perror("open");
            return 1;
        }

        struct msgbuf msg;
        int n;
        while ((n = msgrcv(msqid, &msg, MSGMAX, 0, 0)) > 0) {
            write(out_fd, msg.mtext, n);
        }

        close(out_fd);
        return 0;
    }

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return 1;
    }

    char *buf = malloc(buf_size);
    struct msgbuf msg;
    msg.mtype = 1;
    int n;

    while ((n = read(in_fd, buf, buf_size)) > 0) {
        int sent = 0;
        while (sent < n) {
            int chunk = n - sent;
            if (chunk > MSGMAX)
                chunk = MSGMAX;
            memcpy(msg.mtext, buf + sent, chunk);
            if (msgsnd(msqid, &msg, chunk, 0) == -1) {
                perror("msgsnd");
                break;
            }
            sent += chunk;
        }
    }

    msg.mtype = 1;
    msgsnd(msqid, &msg, 0, 0);

    close(in_fd);
    free(buf);

    wait(NULL);
    msgctl(msqid, IPC_RMID, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec)
                   + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    fprintf(stderr, "buf=%d time=%.3f s\n", buf_size, elapsed);

    return 0;
}