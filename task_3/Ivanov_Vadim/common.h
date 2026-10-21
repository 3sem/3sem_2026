#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

struct options {
    int input;
    int output;
    size_t buffer_size;
};

int open_files(int argc, char **argv, struct options *options);
ssize_t read_retry(int descriptor, void *buffer, size_t size);
int write_all(int descriptor, const void *buffer, size_t size);
int wait_child(pid_t child);
void print_time(const struct timespec *start);

#endif
