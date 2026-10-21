#include "full_duplex_pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_CHUNK_SIZE 64 * 1024
#define MAX_CHUNK_SIZE 16 * 1024 * 1024

static ssize_t readRetry(int fd, void *buffer, size_t size)
{
    ssize_t count;
    do {
        count = read(fd, buffer, size);
    } while (count < 0 && errno == EINTR);

    return count;
}

static int writeAll(int fd, const void *buffer, size_t size)
{
    const unsigned char *data = buffer;
    while (size > 0) {
        ssize_t count = write(fd, data, size);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }
        if (count == 0) {
            errno = EIO;
            return -1;
        }
        data += count;
        size -= (size_t)count;
    }

    return 0;
}

static int setNonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);

    if (flags < 0) {
        return -1;
    }
    flags = flags | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        return -1;
    }
    return 0;
}

static int runChild(FullDuplexPipe *channel, size_t chunk_size)
{
    unsigned char *buffer = malloc(chunk_size);
    if (buffer == NULL) {
        return -1;
    }
    int result = -1;
    for (;;) {
        ssize_t count = channel->actions.receive(channel, buffer, chunk_size);
        if (count < 0) {
            break;
        }
        if (count == 0) {
            result = 0;
            break;
        }
        size_t offset = 0;
        while (offset < (size_t)count) {
            ssize_t sent = channel->actions.send(channel, buffer + offset,
                                                 (size_t)count - offset);
            if (sent <= 0) {
                if (sent == 0) {
                    errno = EIO;
                }
                break;
            }
            offset += (size_t)sent;
        }
        if (offset < (size_t)count) {
            break;
        }
    }
    free(buffer);
    return result;
}

static int transferData(FullDuplexPipe *channel, int input_fd, int output_fd,
                        size_t chunk_size, unsigned char *send_buffer,
                        unsigned char *receive_buffer, uint64_t *sent_total,
                        uint64_t *received_total)
{
    if (setNonblocking(channel->readFd) < 0 ||
        setNonblocking(channel->writeFd) < 0) {
        return -1;
    }

    size_t pending_size = 0;
    size_t sent_offset = 0;
    int input_ended = 0;
    for (;;) {
        if (!input_ended && sent_offset == pending_size) {
            ssize_t count = readRetry(input_fd, send_buffer, chunk_size);
            if (count < 0) {
                return -1;
            }
            pending_size = (size_t)count;
            sent_offset = 0;
            if (count == 0) {
                input_ended = 1;
                channel->actions.closeWrite(channel);
            }
        }

        struct pollfd events[2] = {
            {.fd = channel->readFd, .events = POLLIN},
            {.fd = channel->writeFd, .events = POLLOUT}
        };
        int ready;
        do {
            ready = poll(events, 2, -1);
        } while (ready < 0 && errno == EINTR);
        if (ready < 0) {
            return -1;
        }
        if ((events[0].revents | events[1].revents) & POLLNVAL) {
            errno = EBADF;
            return -1;
        }

        if (events[0].revents & (POLLIN | POLLHUP | POLLERR)) {
            ssize_t count = channel->actions.receive(channel, receive_buffer,
                                                     chunk_size);
            if (count > 0) {
                if (writeAll(output_fd, receive_buffer, (size_t)count) < 0) {
                    return -1;
                }
                *received_total += (uint64_t)count;
            } else if (count == 0) {
                if (!input_ended || *received_total != *sent_total) {
                    errno = EIO;
                    return -1;
                }
                return 0;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
        }

        if (events[1].revents & (POLLERR | POLLHUP)) {
            errno = EPIPE;
            return -1;
        }
        if (events[1].revents & POLLOUT) {
            ssize_t count = channel->actions.send(channel,
                send_buffer + sent_offset, pending_size - sent_offset);
            if (count > 0) {
                sent_offset += (size_t)count;
                *sent_total += (uint64_t)count;
            } else if (count == 0) {
                errno = EIO;
                return -1;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
        }
    }
}

static int runParent(FullDuplexPipe *channel, int input_fd, int output_fd,
                     size_t chunk_size, uint64_t *sent_total,
                     uint64_t *received_total)
{
    unsigned char *send_buffer = malloc(chunk_size);
    unsigned char *receive_buffer = malloc(chunk_size);
    int result = -1;
    if (send_buffer != NULL && receive_buffer != NULL) {
        result = transferData(channel, input_fd, output_fd, chunk_size,
                              send_buffer, receive_buffer, sent_total,
                              received_total);
    }
    free(send_buffer);
    free(receive_buffer);
    return result;
}

static int parseChunkSize(const char *text, size_t *chunk_size)
{
    if (*text < '0' || *text > '9') {
        return -1;
    }
    char *end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || *end != '\0' || value == 0 || value > MAX_CHUNK_SIZE) {
        return -1;
    }
    *chunk_size = (size_t)value;
    return 0;
}

int main(int argc, char **argv)
{
    size_t chunk_size = DEFAULT_CHUNK_SIZE;
    if ((argc != 3 && argc != 4) ||
        (argc == 4 && parseChunkSize(argv[3], &chunk_size) < 0)) {
        fprintf(stderr, "Usage: %s INPUT OUTPUT [CHUNK_BYTES: 1..%d]\n",
                argv[0], MAX_CHUNK_SIZE);
        return EXIT_FAILURE;
    }

    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_FAILURE;
    int input_fd = open(argv[1], O_RDONLY);
    int output_fd = -1;
    if (input_fd < 0) {
        perror("open input");
        return EXIT_FAILURE;
    }
    do {
        struct stat input_info;
        struct stat output_info;
        if (fstat(input_fd, &input_info) < 0) {
            perror("fstat input");
            break;
        }
        if (!S_ISREG(input_info.st_mode)) {
            fprintf(stderr, "Input must be a regular file\n");
            break;
        }
        output_fd = open(argv[2], O_WRONLY | O_CREAT, 0666);
        if (output_fd < 0) {
            perror("open output");
            break;
        }
        if (fstat(output_fd, &output_info) < 0) {
            perror("fstat output");
            break;
        }
        if (!S_ISREG(output_info.st_mode) ||
            (input_info.st_dev == output_info.st_dev &&
             input_info.st_ino == output_info.st_ino)) {
            fprintf(stderr, "Output must be a regular file different from input\n");
            break;
        }
        if (ftruncate(output_fd, 0) < 0) {
            perror("ftruncate");
            break;
        }

        FullDuplexPipe channel;
        if (fullDuplexPipeInit(&channel) < 0) {
            perror("pipe init");
            break;
        }
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("fork");
            channel.actions.destroy(&channel);
            break;
        }
        if (child_pid == 0) {
            close(input_fd);
            close(output_fd);
            int result = channel.actions.selectSide(&channel, PIPE_CHILD);
            if (result == 0) {
                result = runChild(&channel, chunk_size);
            }
            if (result < 0) {
                perror("child echo");
            }
            channel.actions.destroy(&channel);
            _exit(result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
        }

        uint64_t sent_total = 0;
        uint64_t received_total = 0;
        struct timespec start = {0};
        struct timespec finish = {0};
        int result = channel.actions.selectSide(&channel, PIPE_PARENT);
        if (result == 0) {
            result = clock_gettime(CLOCK_MONOTONIC, &start);
        }
        if (result == 0) {
            result = runParent(&channel, input_fd, output_fd, chunk_size,
                               &sent_total, &received_total);
        }
        if (result < 0) {
            perror("parent transfer");
        }
        channel.actions.destroy(&channel);
        if (close(output_fd) < 0) {
            perror("close output");
            result = -1;
        }
        output_fd = -1;

        int child_status = 0;
        pid_t waited;
        do {
            waited = waitpid(child_pid, &child_status, 0);
        } while (waited < 0 && errno == EINTR);
        if (waited < 0) {
            perror("waitpid");
            break;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &finish) < 0) {
            perror("clock_gettime");
            break;
        }
        if (result < 0 || !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
            fprintf(stderr, "Echo test failed\n");
            break;
        }
        double seconds = (double)(finish.tv_sec - start.tv_sec) +
                         (double)(finish.tv_nsec - start.tv_nsec) / 1e9;
        printf("Sent:     %" PRIu64 " bytes\n", sent_total);
        printf("Received: %" PRIu64 " bytes\n", received_total);
        printf("Chunk:    %zu bytes\n", chunk_size);
        printf("Elapsed:  %.6f s\n", seconds);
        if (seconds > 0) {
            printf("Round-trip throughput (2N/t): %.2f MiB/s\n",
                   2.0 * (double)sent_total / (1024.0 * 1024.0) / seconds);
        }
        exit_code = EXIT_SUCCESS;
    } while (0);

    close(input_fd);
    if (output_fd >= 0) {
        close(output_fd);
    }
    return exit_code;
}
