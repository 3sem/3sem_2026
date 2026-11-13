#define _POSIX_C_SOURCE 200809L
#include "common.h"

#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>

struct shared_buffer {
    sem_t empty;
    sem_t full;
    size_t length;
    int failed;
    unsigned char data[];
};

static int wait_semaphore(sem_t *semaphore)
{
    int result;
    do {
        result = sem_wait(semaphore);
    } while (result == -1 && errno == EINTR);
    return result;
}

static int receive_file(struct shared_buffer *shared, int output)
{
    for (;;) {
        if (wait_semaphore(&shared->full) == -1)
            return 1;
        if (shared->length == 0)
            return 0;
        if (write_all(output, shared->data, shared->length) == -1) {
            perror("write destination");
            shared->failed = 1;
            sem_post(&shared->empty);
            return 1;
        }
        sem_post(&shared->empty);
    }
}

int main(int argc, char **argv)
{
    struct options options;
    if (open_files(argc, argv, &options) == -1)
        return 1;
    int memory = shmget(IPC_PRIVATE, sizeof(struct shared_buffer) + options.buffer_size,
                        IPC_CREAT | 0600);
    if (memory == -1) {
        perror("shmget");
        return 1;
    }
    struct shared_buffer *shared = shmat(memory, NULL, 0);
    shmctl(memory, IPC_RMID, NULL);
    if (shared == (void *)-1) {
        perror("shmat");
        return 1;
    }
    if (sem_init(&shared->empty, 1, 1) == -1) {
        perror("sem_init");
        shmdt(shared);
        return 1;
    }
    if (sem_init(&shared->full, 1, 0) == -1) {
        perror("sem_init");
        sem_destroy(&shared->empty);
        shmdt(shared);
        return 1;
    }
    shared->failed = 0;
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    pid_t child = fork();
    int result = 0;
    if (child == -1) {
        perror("fork");
        result = 1;
    } else if (child == 0) {
        close(options.input);
        result = receive_file(shared, options.output);
        if (close(options.output) == -1)
            result = 1;
        shmdt(shared);
        _exit(result);
    } else {
        close(options.output);
        for (;;) {
            if (wait_semaphore(&shared->empty) == -1 || shared->failed) {
                result = 1;
                break;
            }
            ssize_t count = read_retry(options.input, shared->data, options.buffer_size);
            if (count == -1) {
                perror("read source");
                result = 1;
            }
            shared->length = count > 0 ? (size_t)count : 0;
            sem_post(&shared->full);
            if (count <= 0)
                break;
        }
        close(options.input);
        if (wait_child(child) == -1)
            result = 1;
    }
    sem_destroy(&shared->empty);
    sem_destroy(&shared->full);
    shmdt(shared);
    if (result == 0)
        print_time(&start);
    return result;
}
