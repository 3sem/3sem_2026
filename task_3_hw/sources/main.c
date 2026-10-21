#include "transfer.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted = 0;
static volatile sig_atomic_t child_exited = 0;

static void handleSignal(int signal_number) {
    if (signal_number == SIGCHLD) {
        child_exited = 1;
    } else if (signal_number == SIGALRM) {
        /* Interrupt a blocked syscall even if SIGCHLD arrived just before it. */
        alarm(1);
    } else {
        interrupted = 1;
    }
}

int shouldStop(void) {
    if (interrupted || child_exited) {
        errno = ECANCELED;
        return 1;
    }
    return 0;
}

static int installHandlers(void) {
    struct sigaction action = {0};
    action.sa_handler = handleSignal;
    sigemptyset(&action.sa_mask);
    const int signals[] = {SIGINT, SIGTERM, SIGCHLD, SIGALRM};
    for (size_t index = 0; index < sizeof(signals) / sizeof(signals[0]); ++index) {
        if (sigaction(signals[index], &action, NULL) < 0) {
            return -1;
        }
    }
    action.sa_handler = SIG_IGN;
    return sigaction(SIGPIPE, &action, NULL);
}

static int parseBufferSize(const char *argument, size_t *buffer_size) {
    if (*argument == '\0') {
        return -1;
    }
    size_t value = 0;
    for (const char *digit = argument; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9' ||
            value > (MAX_BUFFER_SIZE - (unsigned) (*digit - '0')) / 10U) {
            return -1;
        }
        value = value * 10U + (unsigned) (*digit - '0');
    }
    if (value == 0) {
        return -1;
    }
    *buffer_size = value;
    return 0;
}

static int validateFiles(int input_fd, int output_fd, uint64_t *file_size) {
    struct stat input_stat;
    struct stat output_stat;
    if (fstat(input_fd, &input_stat) < 0 || fstat(output_fd, &output_stat) < 0) {
        return -1;
    }
    if (!S_ISREG(input_stat.st_mode) || !S_ISREG(output_stat.st_mode) ||
        (input_stat.st_dev == output_stat.st_dev &&
         input_stat.st_ino == output_stat.st_ino) || input_stat.st_size < 0) {
        errno = EINVAL;
        return -1;
    }
    *file_size = (uint64_t) input_stat.st_size;
    return 0;
}

static int transferFile(Channel *channel, int file_fd, unsigned char *buffer,
                        size_t buffer_size, uint64_t expected_size, bool sender) {
    if (channelSelectSide(channel, sender) < 0) {
        return -1;
    }
    uint64_t total = 0;
    for (;;) {
        ssize_t count = sender ? readBlock(file_fd, buffer, buffer_size)
                               : channelReceive(channel, buffer, buffer_size);
        if (count < 0) {
            return -1;
        }
        if ((uint64_t) count > expected_size - total) {
            errno = EIO;
            return -1;
        }
        if (count == 0) {
            if (total != expected_size) {
                errno = EIO;
                return -1;
            }
            /* A zero-length packet denotes EOF in SHM/MQ; FIFO closes writer. */
            return sender ? channelSend(channel, buffer, 0) : 0;
        }
        int result = sender ? channelSend(channel, buffer, (size_t) count)
                            : writeAll(file_fd, buffer, (size_t) count);
        if (result < 0) {
            return -1;
        }
        total += (uint64_t) count;
    }
}

static int waitChild(pid_t child_pid, bool failed) {
    int status = 0;
    for (;;) {
        if (failed || interrupted) {
            /* Only our own, not-yet-reaped child is targeted. */
            if (kill(child_pid, SIGKILL) < 0 && errno != ESRCH) {
                return -1;
            }
        }
        pid_t result = waitpid(child_pid, &status, 0);
        if (result >= 0) {
            break;
        }
        if (errno != EINTR) {
            return -1;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || interrupted) {
        errno = ECHILD;
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    size_t buffer_size = 65536;
    if ((argc != 3 && argc != 4) ||
        (argc == 4 && parseBufferSize(argv[3], &buffer_size) < 0)) {
        fprintf(stderr, "Usage: %s INPUT OUTPUT [BUFFER_BYTES: 1..%u]\n",
                argv[0], MAX_BUFFER_SIZE);
        return EXIT_FAILURE;
    }
    if (installHandlers() < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int result = EXIT_FAILURE;
    int input_fd = -1;
    int output_fd = -1;
    Channel *channel = NULL;
    unsigned char *buffer = NULL;
    uint64_t file_size = 0;
    /* O_NONBLOCK prevents hanging on a non-regular input/output before fstat. */
    input_fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        perror("open input");
        goto cleanup;
    }
    output_fd = open(argv[2], O_WRONLY | O_CREAT | O_NONBLOCK, 0600);
    if (output_fd < 0) {
        perror("open output");
        goto cleanup;
    }
    if (validateFiles(input_fd, output_fd, &file_size) < 0) {
        perror("files must be distinct regular files");
        goto cleanup;
    }
    buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc");
        goto cleanup;
    }
    channel = channelCreate(buffer_size);
    if (channel == NULL) {
        perror("channelCreate");
        goto cleanup;
    }
    if (ftruncate(output_fd, 0) < 0) {
        perror("ftruncate");
        goto cleanup;
    }

    struct timespec start;
    struct timespec finish;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime");
        goto cleanup;
    }
    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        goto cleanup;
    }
    if (child_pid == 0) {
        close(input_fd);
        int child_result = transferFile(channel, output_fd, buffer, buffer_size,
                                        file_size, false);
        if (child_result < 0) {
            perror("receive file");
        }
        if (close(output_fd) < 0) {
            perror("close output");
            child_result = -1;
        }
        /* The parent owns IPC removal; exit detaches the child's mappings. */
        free(buffer);
        _exit(child_result == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    close(output_fd);
    output_fd = -1;
    alarm(1);
    int send_result = transferFile(channel, input_fd, buffer, buffer_size,
                                   file_size, true);
    if (send_result < 0) {
        perror("send file");
    }
    int child_result = waitChild(child_pid, send_result < 0);
    alarm(0);
    if (child_result < 0) {
        perror("receiver failed");
    }
    if (clock_gettime(CLOCK_MONOTONIC, &finish) < 0) {
        perror("clock_gettime");
        goto cleanup;
    }
    if (send_result < 0 || child_result < 0) {
        goto cleanup;
    }
    double elapsed = (double) (finish.tv_sec - start.tv_sec) +
                     (double) (finish.tv_nsec - start.tv_nsec) / 1e9;
    printf("method=%s bytes=%" PRIu64 " buffer=%zu seconds=%.9f\n",
           channelName(), file_size, buffer_size, elapsed);
    result = EXIT_SUCCESS;

cleanup:
    if (channel != NULL && channelDestroy(channel) < 0) {
        perror("channelDestroy");
        result = EXIT_FAILURE;
    }
    free(buffer);
    if (input_fd >= 0) {
        close(input_fd);
    }
    if (output_fd >= 0) {
        close(output_fd);
    }
    return result;
}
