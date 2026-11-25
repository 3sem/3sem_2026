#define _POSIX_C_SOURCE 200809L
#include "channel.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE (1024 * 1024)

typedef struct {
    Channel *channel;
    int output_fd;
    char *buffer;
    uint64_t received;
    int error;
} ReceiveInfo;

static int write_file(int fd, const void *data, size_t size)
{
    const char *p = data;
    size_t done = 0;

    while (done < size) {
        ssize_t n = write(fd, p + done, size - done);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        done += (size_t)n;
    }

    return 0;
}

static void *receive_file(void *arg)
{
    ReceiveInfo *info = arg;

    for (;;) {
        ssize_t n = info->channel->ops->receive(
            info->channel, info->buffer, BUF_SIZE);

        if (n == 0)
            break;

        if (n < 0) {
            perror("read from pipe");
            info->error = 1;
            break;
        }

        info->received += (uint64_t)n;

        if (!info->error &&
            write_file(info->output_fd, info->buffer, (size_t)n) == -1) {
            perror("write to output file");
            info->error = 1;
        }
    }

    return NULL;
}

static int child_work(Channel *channel, char *buffer)
{
    for (;;) {
        ssize_t n = channel->ops->receive(channel, buffer, BUF_SIZE);

        if (n == 0)
            break;

        if (n < 0) {
            perror("child read");
            channel->ops->close_all(channel);
            return 1;
        }

        if (channel->ops->send(channel, buffer, (size_t)n) == -1) {
            perror("child write");
            channel->ops->close_all(channel);
            return 1;
        }
    }

    channel->ops->close_all(channel);
    return 0;
}

static double seconds_between(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }

    int input_fd = open(argv[1], O_RDONLY);
    if (input_fd == -1) {
        perror("open input");
        return 1;
    }

    int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("open output");
        close(input_fd);
        return 1;
    }

    char *send_buffer = malloc(BUF_SIZE);
    char *receive_buffer = malloc(BUF_SIZE);
    char *child_buffer = malloc(BUF_SIZE);

    if (send_buffer == NULL || receive_buffer == NULL || child_buffer == NULL) {
        perror("malloc");
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        close(input_fd);
        close(output_fd);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    Channel parent = {-1, -1, NULL};
    Channel child = {-1, -1, NULL};

    if (channel_make(&parent, &child) == -1) {
        perror("pipe");
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        close(input_fd);
        close(output_fd);
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        parent.ops->close_all(&parent);
        child.ops->close_all(&child);
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        close(input_fd);
        close(output_fd);
        return 1;
    }

    if (pid == 0) {
        parent.ops->close_all(&parent);
        close(input_fd);
        close(output_fd);
        int result = child_work(&child, child_buffer);
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        _exit(result);
    }

    child.ops->close_all(&child);

    ReceiveInfo info = {&parent, output_fd, receive_buffer, 0, 0};
    pthread_t reader;
    int thread_error = pthread_create(&reader, NULL, receive_file, &info);

    if (thread_error != 0) {
        errno = thread_error;
        perror("pthread_create");
        parent.ops->close_all(&parent);
        close(input_fd);
        close(output_fd);
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        waitpid(pid, NULL, 0);
        return 1;
    }

    struct timespec start;
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        parent.ops->close_send(&parent);
        pthread_join(reader, NULL);
        parent.ops->close_all(&parent);
        close(input_fd);
        close(output_fd);
        free(send_buffer);
        free(receive_buffer);
        free(child_buffer);
        waitpid(pid, NULL, 0);
        return 1;
    }

    uint64_t sent = 0;
    int error = 0;

    for (;;) {
        ssize_t n = read(input_fd, send_buffer, BUF_SIZE);

        if (n == 0)
            break;

        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("read input");
            error = 1;
            break;
        }

        if (parent.ops->send(&parent, send_buffer, (size_t)n) == -1) {
            perror("send to child");
            error = 1;
            break;
        }

        sent += (uint64_t)n;
    }

    close(input_fd);
    parent.ops->close_send(&parent);

    int join_error = pthread_join(reader, NULL);
    if (join_error != 0) {
        errno = join_error;
        perror("pthread_join");
        error = 1;
    }

    parent.ops->close_all(&parent);
    close(output_fd);

    int status;
    pid_t waited;

    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        perror("waitpid");
        error = 1;
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "child process failed\n");
        error = 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        perror("clock_gettime");
        error = 1;
    }

    if (info.error)
        error = 1;

    printf("Sent:     %" PRIu64 " bytes\n", sent);
    printf("Received: %" PRIu64 " bytes\n", info.received);

    if (sent != info.received) {
        fprintf(stderr, "The number of bytes does not match\n");
        error = 1;
    }

    if (!error)
        printf("Transfer time: %.3f seconds\n", seconds_between(start, end));

    free(send_buffer);
    free(receive_buffer);
    free(child_buffer);

    return error ? 1 : 0;
}
