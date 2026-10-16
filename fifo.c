#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FIFO_NAME "myfifo"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s input output [bufsize]\n", argv[0]);
        return 1;
    }

    int buf_size = (argc >= 4) ? atoi(argv[3]) : 4096;

    unlink(FIFO_NAME);
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("mkfifo");
        return 1;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        int fifo_fd = open(FIFO_NAME, O_RDONLY);
        int out_fd  = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fifo_fd == -1 || out_fd == -1) {
            perror("open");
            return 1;
        }

        char *buf = malloc(buf_size);
        int n;
        while ((n = read(fifo_fd, buf, buf_size)) > 0) {
            write(out_fd, buf, n);
        }

        close(fifo_fd);
        close(out_fd);
        free(buf);
        return 0;
    }

    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    int in_fd   = open(argv[1], O_RDONLY);
    if (fifo_fd == -1 || in_fd == -1) {
        perror("open");
        return 1;
    }

    char *buf = malloc(buf_size);
    int n;
    while ((n = read(in_fd, buf, buf_size)) > 0) {
        write(fifo_fd, buf, n);
    }

    close(fifo_fd);
    close(in_fd);
    free(buf);

    wait(NULL);
    unlink(FIFO_NAME);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec)
                   + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    fprintf(stderr, "buf=%d time=%.3f s\n", buf_size, elapsed);

    return 0;
}