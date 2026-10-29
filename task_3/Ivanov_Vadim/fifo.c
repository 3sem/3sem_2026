#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int copy_stream(int input, int output, unsigned char *buffer, size_t size)
{
    ssize_t count;
    while ((count = read_retry(input, buffer, size)) > 0) {
        if (write_all(output, buffer, (size_t)count) == -1) {
            perror("write");
            return 1;
        }
    }
    if (count == -1) {
        perror("read");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct options options;
    if (open_files(argc, argv, &options) == -1)
        return 1;
    unsigned char *buffer = malloc(options.buffer_size);
    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }
    char directory[] = "/tmp/task3_fifo_XXXXXX";
    if (mkdtemp(directory) == NULL) {
        perror("mkdtemp");
        free(buffer);
        return 1;
    }
    char path[128];
    snprintf(path, sizeof(path), "%s/pipe", directory);
    if (mkfifo(path, 0600) == -1) {
        perror("mkfifo");
        rmdir(directory);
        free(buffer);
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pid_t child = fork();
    int result = 0;
    if (child == -1) {
        perror("fork");
        result = 1;
    } else if (child == 0) {
        close(options.input);
        int channel = open(path, O_RDONLY);
        if (channel == -1) {
            perror("open fifo");
            _exit(1);
        }
        result = copy_stream(channel, options.output, buffer, options.buffer_size);
        close(channel);
        if (close(options.output) == -1)
            result = 1;
        free(buffer);
        _exit(result);
    } else {
        close(options.output);
        int channel = open(path, O_WRONLY);
        if (channel == -1) {
            perror("open fifo");
            kill(child, SIGTERM);
            result = 1;
        } else {
            result = copy_stream(options.input, channel, buffer, options.buffer_size);
            close(channel);
        }
        close(options.input);
        if (wait_child(child) == -1)
            result = 1;
    }
    unlink(path);
    rmdir(directory);
    free(buffer);
    if (result == 0)
        print_time(&start);
    return result;
}
