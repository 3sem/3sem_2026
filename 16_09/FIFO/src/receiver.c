#include "fifo.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int receiveFile(int fifo_fd, FILE *output, size_t buffer_size) {
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("malloc output buffer");
        return -1;
    }

    int result = 0;

    while (true) {
        ssize_t bytes_read;

        do {
            bytes_read = read(fifo_fd, buffer, buffer_size);
        } while (bytes_read == -1 && errno == EINTR);

        if (bytes_read == -1) {
            perror("read FIFO");
            result = -1;
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        size_t bytes_to_write = (size_t)bytes_read;

        if (fwrite(buffer, 1, bytes_to_write, output) != bytes_to_write) {
            perror("fwrite output");
            result = -1;
            break;
        }
    }

    free(buffer);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    const size_t buffer_size = 64 * 1024;

    FILE *output = fopen(argv[1], "wb");
    if (output == NULL) {
        perror("fopen output");
        return 1;
    }

    if (prepareFifo(FIFO_PATH) == -1) {
        fclose(output);
        return 1;
    }

    int fifo_fd = openFifoForReading(FIFO_PATH);
    if (fifo_fd == -1) {
        fclose(output);
        return 1;
    }

    int result = receiveFile(fifo_fd, output, buffer_size);

    /* Include durable output completion in the measured transfer time. */
    if (result == 0 && fflush(output) == EOF) {
        perror("fflush output");
        result = -1;
    }

    if (result == 0 && fsync(fileno(output)) == -1) {
        perror("fsync output");
        result = -1;
    }

    if (close(fifo_fd) == -1) {
        perror("close FIFO");
        result = -1;
    }

    if (fclose(output) == EOF) {
        perror("fclose output");
        result = -1;
    }

    if (removeFifo(FIFO_PATH) == -1) {
        result = -1;
    }

    return result == 0 ? 0 : 1;
}

