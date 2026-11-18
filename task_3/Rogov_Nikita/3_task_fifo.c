#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_PATH "/tmp/my_test_fifo"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Использование: %s <input> <output> <buf_size_bytes>\n", argv[0]);
        return 1;
    }

    char* input_file = argv[1];
    char* output_file = argv[2];
    size_t buf_size = atoll(argv[3]);

    unlink(FIFO_PATH); //delete old fifo

    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return 1;
    }

    char* buffer = (char*) calloc(sizeof(char), buf_size);
    if (!buffer) {
        perror("malloc");
        unlink(FIFO_PATH);
        return 1;
    }

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) {
        perror("open input");
        unlink(FIFO_PATH);
        return 1;
    }

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
        perror("open output");
        close(in_fd);
        unlink(FIFO_PATH);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        free(buffer);
        unlink(FIFO_PATH);
        return 1;
    }

    if (pid == 0) { //child process
        close(in_fd);

        int fifo_fd = open(FIFO_PATH, O_RDONLY);
        if (fifo_fd < 0) {
            perror("open fifo for read");
            exit(1);
        }

        ssize_t bytes_read = 0;
        while ((bytes_read = read(fifo_fd, buffer, buf_size)) > 0) {
            write(out_fd, buffer, bytes_read);
        }

        close(fifo_fd);
        close(out_fd);
        free(buffer);
        exit(0);
    }
    else { //parent process
        close(out_fd);

        int fifo_fd = open(FIFO_PATH, O_WRONLY);
        if (fifo_fd < 0) {
            perror("open fifo for write");
            exit(1);
        }

        ssize_t bytes_read = 0;
        while ((bytes_read = read(in_fd, buffer, buf_size)) > 0) {
            write(fifo_fd, buffer, bytes_read);
        }

        close(fifo_fd);
        close(in_fd);

        wait(NULL);

        unlink(FIFO_PATH);
        free(buffer);
    }

    return 0;
}
