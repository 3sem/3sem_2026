#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

static int read_all(int fd, void *buf, size_t size) {
    size_t done = 0;
    while (done < size) {
        ssize_t n = read(fd, (char *)buf + done, size - done);
        if (n == 0) return (int)done;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)n;
    }
    return (int)done;
}

static int write_all(int fd, const void *buf, size_t size) {
    size_t done = 0;
    while (done < size) {
        ssize_t n = write(fd, (const char *)buf + done, size - done);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static size_t get_size(const char *s) {
    char *end;
    unsigned long long n = strtoull(s, &end, 10);
    if (*s == '\0' || *end != '\0' || n == 0 || n > SIZE_MAX) {
        fprintf(stderr, "invalid buffer size: %s\n", s);
        exit(2);
    }
    return (size_t)n;
}

static double now_sec(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        exit(1);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int open_input(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) perror(path);
    return fd;
}

static int open_output(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) perror(path);
    return fd;
}

#endif
