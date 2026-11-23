#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "duplex.h"

#define CHUNK_SIZE   (4u * 1024 * 1024)  

typedef struct {
    Duplex* d;
    int     in_fd;
    size_t  total;
} SenderArgs;

static void* sender_thread(void* arg)
{
    SenderArgs* a = (SenderArgs*)arg;
    size_t remaining = a->total;
    while (remaining > 0) {
        size_t chunk = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
        ssize_t n = a->d->op_table.SendFile(a->d, a->in_fd, chunk);
        if (n <= 0) break;
        remaining -= (size_t)n;
    }
    return NULL;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN); 

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) { perror("open input"); return 1; }

    struct stat st;
    if (fstat(in_fd, &st) < 0) { perror("fstat"); return 1; }
    size_t total = (size_t)st.st_size;

    int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) { perror("open output"); return 1; }

    Pair pair = Construct(1024u * 1024u);
    fprintf(stderr, "[parent] pipe size = %zu bytes\n", pair.duplex_2->pipe_size);

    double t0 = now_sec();

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(pair.duplex_2->in);
        close(pair.duplex_2->out);
        free(pair.duplex_2);

        Duplex* d = pair.duplex_1;
        ssize_t moved = d->op_table.Echo(d, total);
        if (moved < 0 || (size_t)moved != total) {
            fprintf(stderr, "[child] echo moved %zd of %zu bytes\n", moved, total);
            Destruct(d);
            _exit(2);
        }
        Destruct(d);
        _exit(0);
    }

    close(pair.duplex_1->in);
    close(pair.duplex_1->out);
    free(pair.duplex_1);

    Duplex* d = pair.duplex_2;

    SenderArgs sa = { d, in_fd, total };
    pthread_t tid;
    if (pthread_create(&tid, NULL, sender_thread, &sa) != 0) {
        perror("pthread_create"); return 1;
    }

    size_t remaining = total;
    while (remaining > 0) {
        size_t chunk = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
        ssize_t n = d->op_table.RecvFile(d, out_fd, chunk);
        if (n <= 0) {
            fprintf(stderr, "[parent] recv failed after %zu/%zu bytes\n",
                    total - remaining, total);
            break;
        }
        remaining -= (size_t)n;
    }

    pthread_join(tid, NULL);

    close(in_fd);
    close(out_fd);
    Destruct(d);

    int status = 0;
    waitpid(pid, &status, 0);

    double elapsed = now_sec() - t0;
    double mb = (double)total / (1024.0 * 1024.0);
    fprintf(stderr, "\n=== ECHO TEST DONE ===\n");
    fprintf(stderr, "size:      %.2f MiB\n", mb);
    fprintf(stderr, "elapsed:   %.3f s\n", elapsed);
    fprintf(stderr, "throughput: %.2f MiB/s (round-trip %.2f MiB/s)\n",
            mb / elapsed, (2 * mb) / elapsed);
    fprintf(stderr, "child exit status: %d\n", WEXITSTATUS(status));

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "md5sum '%s' '%s'", argv[1], argv[2]);
        fflush(stderr);
        system(cmd);
    }
    return 0;
}