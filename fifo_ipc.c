// fifo_ipc.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/my_test_fifo"

int main(int argc, char *argv[]) {
    if (argc < 4) return 1;
    char *file_in = argv[1];
    char *file_out = argv[2];
    size_t buf_size = atoi(argv[3]);

    mkfifo(FIFO_PATH, 0666);

    pid_t pid = fork();
    if (pid == 0) { // Потомок - Читатель из FIFO
        int fd_fifo = open(FIFO_PATH, O_RDONLY);
        FILE *out = fopen(file_out, "wb");
        char *buf = malloc(buf_size);
        ssize_t read_bytes;

        while ((read_bytes = read(fd_fifo, buf, buf_size)) > 0) {
            fwrite(buf, 1, read_bytes, out);
        }
        
        free(buf);
        fclose(out);
        close(fd_fifo);
        return 0;
    } else { // Родитель - Писатель в FIFO
        int fd_fifo = open(FIFO_PATH, O_WRONLY);
        FILE *in = fopen(file_in, "rb");
        char *buf = malloc(buf_size);
        size_t read_bytes;

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        while ((read_bytes = fread(buf, 1, buf_size, in)) > 0) {
            write(fd_fifo, buf, read_bytes);
        }

        free(buf);
        fclose(in);
        close(fd_fifo);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("FIFO Time (%zu bytes): %.4f sec\n", buf_size, elapsed);
        unlink(FIFO_PATH);
    }
    return 0;
}
