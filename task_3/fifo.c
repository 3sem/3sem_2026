#include "common.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#ifndef F_GETPIPE_SZ
#define F_GETPIPE_SZ 1032
#endif

static int pipe_size(void) {
    char path[128];
    snprintf(path, sizeof(path), "/tmp/task3_size_%ld", (long)getpid());
    if (mkfifo(path, 0600) == -1) { perror("mkfifo"); return -1; }
    int fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd == -1) { perror("open fifo"); unlink(path); return -1; }
    int n = fcntl(fd, F_GETPIPE_SZ);
    if (n == -1) perror("F_GETPIPE_SZ");
    close(fd); unlink(path);
    return n;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--pipe-size") == 0) {
        int n = pipe_size();
        if (n < 0) return 1;
        printf("%d\n", n);
        return 0;
    }
    if (argc != 4) {
        fprintf(stderr, "usage: %s input output buffer_size\n", argv[0]);
        return 2;
    }
    size_t cap = get_size(argv[3]);
    int in = open_input(argv[1]);
    if (in == -1) return 1;
    int out = open_output(argv[2]);
    if (out == -1) { close(in); return 1; }

    char path[128];
    snprintf(path, sizeof(path), "/tmp/task3_fifo_%ld", (long)getpid());
    if (mkfifo(path, 0600) == -1) { perror("mkfifo"); return 1; }
    double start = now_sec();
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); unlink(path); return 1; }
    if (pid == 0) {
        int rd = open(path, O_RDONLY);
        if (rd == -1) _exit(2);
        char *buf = malloc(cap);
        if (!buf) _exit(3);
        for (;;) {
            int n = read_all(rd, buf, cap);
            if (n < 0) _exit(4);
            if (n == 0) break;
            if (write_all(out, buf, (size_t)n) == -1) _exit(5);
        }
        free(buf); close(rd); close(out); close(in); _exit(0);
    }

    int wr = open(path, O_WRONLY);
    int failed = wr == -1;
    if (failed) perror("open fifo writer");
    char *buf = malloc(cap);
    if (!buf) { perror("malloc"); failed = 1; }
    if (!failed) {
        for (;;) {
            int n = read_all(in, buf, cap);
            if (n < 0) { perror("read"); failed = 1; break; }
            if (n == 0) break;
            if (write_all(wr, buf, (size_t)n) == -1) { perror("write fifo"); failed = 1; break; }
        }
    }
    free(buf);
    if (wr != -1) close(wr);
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) { perror("waitpid"); failed = 1; }
    double elapsed = now_sec() - start;
    printf("%.6f\n", elapsed);
    close(in); close(out); unlink(path);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failed = 1;
    return failed;
}
