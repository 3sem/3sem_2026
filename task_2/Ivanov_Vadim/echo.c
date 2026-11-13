#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include "duplex.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    DuplexPipe *channel;
    int input;
    size_t chunk_size;
    uint64_t bytes;
    int error;
} Sender;

static int write_all(int descriptor, const void *buffer, size_t size)
{
    const char *position = buffer;
    while (size > 0) {
        ssize_t written = write(descriptor, position, size);
        if (written < 0 && errno == EINTR)
            continue;
        if (written <= 0)
            return -1;
        position += written;
        size -= (size_t)written;
    }
    return 0;
}

static void *send_file(void *argument)
{
    Sender *sender = argument;
    char *buffer = malloc(sender->chunk_size);
    if (buffer == NULL) {
        sender->error = ENOMEM;
        sender->channel->methods->finish(sender->channel);
        return NULL;
    }
    while (1) {
        ssize_t received = read(sender->input, buffer, sender->chunk_size);
        if (received < 0 && errno == EINTR)
            continue;
        if (received < 0) {
            sender->error = errno;
            break;
        }
        if (received == 0)
            break;
        if (sender->channel->methods->send(sender->channel, buffer,
                                            (size_t)received) < 0) {
            sender->error = errno;
            break;
        }
        sender->bytes += (uint64_t)received;
    }
    free(buffer);
    sender->channel->methods->finish(sender->channel);
    return NULL;
}

static int echo_child(DuplexPipe *channel, size_t chunk_size)
{
    char *buffer = malloc(chunk_size);
    if (buffer == NULL)
        return 1;
    int result = 0;
    while (1) {
        ssize_t received = channel->methods->receive(channel, buffer, chunk_size);
        if (received == 0)
            break;
        if (received < 0 || channel->methods->send(channel, buffer,
                                                  (size_t)received) < 0) {
            perror("child echo");
            result = 1;
            break;
        }
    }
    free(buffer);
    channel->methods->close(channel);
    return result;
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s INPUT OUTPUT [CHUNK_BYTES]\n", argv[0]);
        return 1;
    }
    size_t chunk_size = 1024 * 1024;
    if (argc == 4) {
        char *end;
        errno = 0;
        unsigned long value = strtoul(argv[3], &end, 10);
        if (errno || *end || end == argv[3] || value == 0 ||
            value > 64UL * 1024 * 1024) {
            fputs("Chunk size must be 1..67108864 bytes\n", stderr);
            return 1;
        }
        chunk_size = (size_t)value;
    }

    int input = open(argv[1], O_RDONLY);
    if (input < 0) {
        perror("open input");
        return 1;
    }
    int output = open(argv[2], O_WRONLY | O_CREAT, 0666);
    if (output < 0) {
        perror("open output");
        close(input);
        return 1;
    }
    struct stat input_stat;
    struct stat output_stat;
    if (fstat(input, &input_stat) < 0 || fstat(output, &output_stat) < 0) {
        perror("fstat");
        close(input);
        close(output);
        return 1;
    }
    if (input_stat.st_dev == output_stat.st_dev &&
        input_stat.st_ino == output_stat.st_ino) {
        fputs("Input and output must be different files\n", stderr);
        close(input);
        close(output);
        return 1;
    }
    if (ftruncate(output, 0) < 0) {
        perror("ftruncate");
        close(input);
        close(output);
        return 1;
    }
    char *buffer = malloc(chunk_size);
    DuplexPipe endpoints[2];
    if (buffer == NULL || duplex_pair(endpoints) < 0) {
        perror("initialize channel");
        free(buffer);
        close(input);
        close(output);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    struct timespec started;
    struct timespec finished;
    clock_gettime(CLOCK_MONOTONIC, &started);
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        endpoints[0].methods->close(&endpoints[0]);
        endpoints[1].methods->close(&endpoints[1]);
        free(buffer);
        close(input);
        close(output);
        return 1;
    }
    if (child == 0) {
        free(buffer);
        close(input);
        close(output);
        endpoints[0].methods->close(&endpoints[0]);
        _exit(echo_child(&endpoints[1], chunk_size));
    }
    endpoints[1].methods->close(&endpoints[1]);
    DuplexPipe *channel = &endpoints[0];
    Sender sender = {channel, input, chunk_size, 0, 0};
    pthread_t thread;
    int thread_error = pthread_create(&thread, NULL, send_file, &sender);
    int result = 0;
    uint64_t bytes = 0;
    if (thread_error != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(thread_error));
        channel->methods->close(channel);
        result = 1;
    } else {
        while (1) {
            ssize_t received = channel->methods->receive(channel, buffer, chunk_size);
            if (received == 0)
                break;
            if (received < 0 || write_all(output, buffer, (size_t)received) < 0) {
                perror("receive file");
                result = 1;
                break;
            }
            bytes += (uint64_t)received;
        }
        close(channel->read_fd);
        channel->read_fd = -1;
        pthread_join(thread, NULL);
        if (sender.error != 0) {
            fprintf(stderr, "send file: %s\n", strerror(sender.error));
            result = 1;
        }
        if (sender.bytes != bytes)
            result = 1;
        channel->methods->close(channel);
    }

    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    if (waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        result = 1;
    clock_gettime(CLOCK_MONOTONIC, &finished);
    double seconds = (double)(finished.tv_sec - started.tv_sec) +
                     (double)(finished.tv_nsec - started.tv_nsec) / 1e9;
    if (close(output) < 0) {
        perror("close output");
        result = 1;
    }
    close(input);
    free(buffer);
    if (result == 0)
        printf("bytes=%" PRIu64 " time=%.6f s round_trip=%.2f MiB/s\n",
               bytes, seconds, seconds > 0 ? 2.0 * (double)bytes / seconds / 1048576.0 : 0);
    return result;
}
