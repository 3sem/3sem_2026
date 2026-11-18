#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE (4 * 1024 * 1024)
#endif

typedef struct Duplex Duplex;
struct Duplex {
    int input, output;
    ssize_t (*receive)(Duplex *self, void *buffer, size_t size);
    int (*send)(Duplex *self, const void *buffer, size_t size);
    void (*destroy)(Duplex *self);
};

static ssize_t read_block(int fd, void *buffer, size_t size)
{
    size_t done = 0;
    while (done < size) {
        ssize_t n = read(fd, (char *)buffer + done, size - done);
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1)
            return -1;
        if (n == 0)
            break;
        done += (size_t)n;
    }
    return (ssize_t)done;
}

static int write_all(int fd, const void *buffer, size_t size)
{
    size_t done = 0;
    while (done < size) {
        ssize_t n = write(fd, (const char *)buffer + done, size - done);
        if (n == -1 && errno == EINTR)
            continue;
        if (n <= 0) {
            if (n == 0)
                errno = EIO;
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static ssize_t duplex_receive(Duplex *self, void *buffer, size_t size)
{
    return read_block(self->input, buffer, size);
}

static int duplex_send(Duplex *self, const void *buffer, size_t size)
{
    return write_all(self->output, buffer, size);
}

static void duplex_destroy(Duplex *self)
{
    if (self->input != -1)
        close(self->input);
    if (self->output != -1)
        close(self->output);
    self->input = self->output = -1;
}

static int duplex_create(Duplex ends[2])
{
    int forward[2], backward[2];
    if (pipe(forward) == -1)
        return -1;
    if (pipe(backward) == -1) {
        int error = errno;
        close(forward[0]);
        close(forward[1]);
        errno = error;
        return -1;
    }
    ends[0] = (Duplex){backward[0], forward[1], duplex_receive,
                      duplex_send, duplex_destroy};
    ends[1] = (Duplex){forward[0], backward[1], duplex_receive,
                      duplex_send, duplex_destroy};
    return 0;
}

static int receive_exact(Duplex *channel, void *buffer, size_t size)
{
    ssize_t n = channel->receive(channel, buffer, size);
    if (n == (ssize_t)size)
        return 0;
    if (n >= 0)
        errno = EIO;
    return -1;
}

static int echo(Duplex *channel, void *buffer)
{
    uint32_t size;
    ssize_t n;
    while ((n = channel->receive(channel, &size, sizeof(size))) != 0) {
        if (n == -1)
            return -1;
        if (n != sizeof(size) || size == 0 || size > BLOCK_SIZE) {
            errno = EIO;
            return -1;
        }
        /* Сначала принимаем блок целиком, затем возвращаем его родителю. */
        if (receive_exact(channel, buffer, size) == -1 ||
            channel->send(channel, buffer, size) == -1)
            return -1;
    }
    return 0;
}

static void fail(const char *message)
{
    perror(message);
    exit(1);
}

static double now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == -1)
        fail("clock_gettime");
    return (double)t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input_file output_file\n", argv[0]);
        return 1;
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        fail("signal");
    int input = open(argv[1], O_RDONLY);
    if (input == -1)
        fail("input");
    /* O_EXCL защищает исходный и другие уже существующие файлы от перезаписи. */
    int output = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (output == -1)
        fail("output (use a new file)");
    unsigned char *buffer = malloc(BLOCK_SIZE);
    if (buffer == NULL)
        fail("malloc");
    Duplex ends[2];
    if (duplex_create(ends) == -1)
        fail("pipe");

    double start = now();
    pid_t pid = fork();
    if (pid == -1)
        fail("fork");
    if (pid == 0) {
        close(input);
        close(output);
        ends[0].destroy(&ends[0]);
        int result = echo(&ends[1], buffer);
        if (result == -1)
            perror("echo");
        ends[1].destroy(&ends[1]);
        free(buffer);
        _exit(result == -1 ? 1 : 0);
    }
    ends[1].destroy(&ends[1]);
    uint64_t total = 0;
    ssize_t n;
    int failed = 0;
    while ((n = read_block(input, buffer, BLOCK_SIZE)) > 0) {
        uint32_t size = (uint32_t)n;
        if (ends[0].send(&ends[0], &size, sizeof(size)) == -1 ||
            ends[0].send(&ends[0], buffer, size) == -1 ||
            receive_exact(&ends[0], buffer, size) == -1 ||
            write_all(output, buffer, size) == -1) {
            failed = 1;
            break;
        }
        total += size;
    }
    if (n == -1 || failed) {
        perror("transfer");
        failed = 1;
    }
    ends[0].destroy(&ends[0]); /* EOF сообщает потомку об окончании передачи. */
    int status;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Child process failed\n");
        failed = 1;
    }
    close(input);
    if (close(output) == -1) {
        perror("close output");
        failed = 1;
    }
    double elapsed = now() - start;
    if (!failed)
        printf("Echo: %llu bytes, %.3f seconds\n", (unsigned long long)total, elapsed);
    free(buffer);
    return failed;
}
