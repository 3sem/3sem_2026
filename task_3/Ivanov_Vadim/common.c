#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int open_files(int argc, char **argv, struct options *options)
{
    options->buffer_size = 65536;
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s SOURCE DESTINATION [BUFFER_BYTES]\n", argv[0]);
        return -1;
    }
    if (argc == 4) {
        char *end;
        errno = 0;
        unsigned long long value = strtoull(argv[3], &end, 10);
        if (errno || end == argv[3] || *end || value == 0 || value > 67108864) {
            fprintf(stderr, "Buffer size must be between 1 and 67108864 bytes\n");
            return -1;
        }
        options->buffer_size = (size_t)value;
    }
    options->input = open(argv[1], O_RDONLY);
    if (options->input == -1) {
        perror("open source");
        return -1;
    }
    options->output = open(argv[2], O_WRONLY | O_CREAT, 0666);
    if (options->output == -1) {
        perror("open destination");
        close(options->input);
        return -1;
    }
    struct stat source, destination;
    if (fstat(options->input, &source) == -1 || fstat(options->output, &destination) == -1) {
        perror("fstat");
        goto fail;
    }
    if (source.st_dev == destination.st_dev && source.st_ino == destination.st_ino) {
        fprintf(stderr, "Source and destination must be different files\n");
        goto fail;
    }
    if (ftruncate(options->output, 0) == -1) {
        perror("ftruncate");
        goto fail;
    }
    return 0;

fail:
    close(options->input);
    close(options->output);
    return -1;
}

ssize_t read_retry(int descriptor, void *buffer, size_t size)
{
    ssize_t count;
    do {
        count = read(descriptor, buffer, size);
    } while (count == -1 && errno == EINTR);
    return count;
}

int write_all(int descriptor, const void *buffer, size_t size)
{
    const unsigned char *position = buffer;
    while (size > 0) {
        ssize_t count = write(descriptor, position, size);
        if (count == -1 && errno == EINTR)
            continue;
        if (count <= 0)
            return -1;
        position += count;
        size -= (size_t)count;
    }
    return 0;
}

int wait_child(pid_t child)
{
    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return -1;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

void print_time(const struct timespec *start)
{
    struct timespec finish;
    clock_gettime(CLOCK_MONOTONIC, &finish);
    double seconds = finish.tv_sec - start->tv_sec;
    seconds += (finish.tv_nsec - start->tv_nsec) / 1000000000.0;
    printf("time_seconds=%.6f\n", seconds);
}
