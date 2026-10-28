#ifndef COMMON_H
#define COMMON_H
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int input, output;
static size_t block_size;
static pid_t child = -1;
static void cleanup(void);

static void fail(const char *message)
{
    perror(message);
    cleanup(); /* Удаление очереди/семафоров будит второй процесс при ошибке. */
    if (child > 0) {
        kill(child, SIGKILL);
        while (waitpid(child, NULL, 0) == -1 && errno == EINTR) {}
    }
    exit(1);
}

static void prepare(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input output buffer_bytes\n", argv[0]);
        exit(1);
    }
    char *end;
    errno = 0;
    unsigned long long size = strtoull(argv[3], &end, 10);
    if (errno || *end || argv[3][0] == '-' || size == 0 || size > SIZE_MAX / 2) {
        fprintf(stderr, "Invalid buffer size\n");
        exit(1);
    }
    block_size = (size_t)size;
    if (atexit(cleanup) != 0)
        exit(1);
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        fail("signal");
    input = open(argv[1], O_RDONLY);
    if (input == -1)
        fail("input");
    output = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (output == -1)
        fail("output (use a new file)");
}

static ssize_t read_block(int fd, void *buffer, size_t size)
{
    size_t used = 0;
    while (used < size) {
        ssize_t n = read(fd, (char *)buffer + used, size - used);
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1)
            fail("read");
        if (n == 0)
            break;
        used += (size_t)n;
    }
    return (ssize_t)used;
}

static void write_all(int fd, const void *buffer, size_t size)
{
    size_t used = 0;
    while (used < size) {
        ssize_t n = write(fd, (const char *)buffer + used, size - used);
        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0) {
            if (n == 0) errno = EIO;
            fail("write");
        }
        used += (size_t)n;
    }
}

static double now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == -1)
        fail("clock_gettime");
    return (double)t.tv_sec + t.tv_nsec / 1e9;
}

static void finish(double start)
{
    int status;
    pid_t result;
    do {
        result = waitpid(child, &status, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        fail("waitpid");
    child = -1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Receiver failed\n");
        exit(1);
    }
    printf("%.6f\n", now() - start);
}
#endif
