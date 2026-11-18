#include "fifo.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int parseBufferSize(const char *text, size_t *buffer_size) {
    if (text == NULL || *text == '\0') {
        fprintf(stderr, "Buffer size must be a positive integer\n");
        return -1;
    }

    for (const char *digit = text; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9') {
            fprintf(stderr, "Invalid buffer size: %s\n", text);
            return -1;
        }
    }

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(text, &end, 10);

    if (errno == ERANGE || end == text || *end != '\0' ||
        value == 0 || value > SIZE_MAX) {
        fprintf(stderr, "Invalid buffer size: %s\n", text);
        return -1;
    }

    *buffer_size = (size_t)value;
    return 0;
}

static int sendFile(int fifo_fd, FILE *input, size_t buffer_size) {
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc input buffer");
        return -1;
    }

    int result = 0;

    while (true) {
        size_t bytes_read = fread(buffer, 1, buffer_size, input);

        if (ferror(input)) {
            perror("fread");
            result = -1;
            break;
        }

        if (bytes_read > 0 &&
            writeAll(fifo_fd, buffer, bytes_read) == -1) {
            result = -1;
            break;
        }

        if (feof(input)) {
            break;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "fread returned no data without EOF\n");
            result = -1;
            break;
        }
    }

    free(buffer);
    return result;
}

static int ignoreSigpipe(void) {
    struct sigaction action = {0};
    action.sa_handler = SIG_IGN;

    if (sigemptyset(&action.sa_mask) == -1) {
        perror("sigemptyset");
        return -1;
    }

    if (sigaction(SIGPIPE, &action, NULL) == -1) {
        perror("sigaction SIGPIPE");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <buffer_size>\n", argv[0]);
        return 1;
    }

    size_t buffer_size;
    if (parseBufferSize(argv[2], &buffer_size) == -1) {
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("fopen input");
        return 1;
    }

    if (ignoreSigpipe() == -1 || prepareFifo(FIFO_PATH) == -1) {
        fclose(input);
        return 1;
    }

    int fifo_fd = openFifoForWriting(FIFO_PATH);
    if (fifo_fd == -1) {
        fclose(input);
        return 1;
    }

    int result = sendFile(fifo_fd, input, buffer_size);

    if (fclose(input) == EOF) {
        perror("fclose input");
        result = -1;
    }

    if (close(fifo_fd) == -1) {
        perror("close FIFO");
        result = -1;
    }

    return result == 0 ? 0 : 1;
}
