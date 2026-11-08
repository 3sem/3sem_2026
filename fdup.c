#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE (64 * 1024)

typedef struct FullDuplexPipe {
    int p2c[2]; // p2c - parent to child
    int c2p[2];

    int (*init)(struct FullDuplexPipe* self);
    void (*prepare_parent)(struct FullDuplexPipe* self);
    void (*prepare_child)(struct FullDuplexPipe* self);
    ssize_t (*write_to_child)(struct FullDuplexPipe* self, const void* buf, size_t count);
    ssize_t (*read_from_child)(struct FullDuplexPipe* self, void* buf, size_t count);
    ssize_t (*write_to_parent)(struct FullDuplexPipe* self, const void* buf, size_t count);
    ssize_t (*read_from_parent)(struct FullDuplexPipe* self, void* buf, size_t count);
    void (*close_all)(struct FullDuplexPipe* self);
} FullDuplexPipe;

int fdp_init(FullDuplexPipe* self) {
    if (pipe(self->p2c) < 0) return -1;
    if (pipe(self->c2p) < 0) {
        close(self->p2c[0]);
        close(self->p2c[1]);
        return -1;
    }
    return 0;
}

void fdp_prepare_parent(FullDuplexPipe* self) {
    close(self->p2c[0]); 
    close(self->c2p[1]); 

    self->c2p [1] = -1;
    self->p2c [0] = -1;
}

void fdp_prepare_child(FullDuplexPipe* self) {
    close(self->p2c[1]); 
    close(self->c2p[0]); 

    self->c2p [0] = -1;
    self->p2c [1] = -1;
}

ssize_t fdp_write_to_child(FullDuplexPipe* self, const void* buf, size_t count) {
    return write(self->p2c[1], buf, count);
}

ssize_t fdp_read_from_child(FullDuplexPipe* self, void* buf, size_t count) {
    return read(self->c2p[0], buf, count);
}

ssize_t fdp_write_to_parent(FullDuplexPipe* self, const void* buf, size_t count) {
    return write(self->c2p[1], buf, count);
}

ssize_t fdp_read_from_parent(FullDuplexPipe* self, void* buf, size_t count) {
    return read(self->p2c[0], buf, count);
}

void fdp_close_all(FullDuplexPipe* self) {
    close(self->p2c[0]); close(self->p2c[1]);
    close(self->c2p[0]); close(self->c2p[1]);
}

FullDuplexPipe create_full_duplex_pipe() {
    FullDuplexPipe pipe_obj = {
        .init = fdp_init,
        .prepare_parent = fdp_prepare_parent,
        .prepare_child = fdp_prepare_child,
        .write_to_child = fdp_write_to_child,
        .read_from_child = fdp_read_from_child,
        .write_to_parent = fdp_write_to_parent,
        .read_from_parent = fdp_read_from_parent,
        .close_all = fdp_close_all
    };
    return pipe_obj;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <входной_файл> <выходной_файл>\n", argv[0]);
        return 1;
    }

    FullDuplexPipe pipe = create_full_duplex_pipe();
    
    if (pipe.init(&pipe) < 0) {
        perror("Ошибка инициализации пайпа");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Ошибка fork");
        pipe.close_all(&pipe);
        return 1;
    }

    if (pid == 0) {
        //child
        pipe.prepare_child(&pipe);
        char* child_buf = malloc(BUFFER_SIZE);
        ssize_t bytes_read;

        while ((bytes_read = pipe.read_from_parent(&pipe, child_buf, BUFFER_SIZE)) > 0) {
            ssize_t bytes_written = 0;
            while (bytes_written < bytes_read) {
                ssize_t res = pipe.write_to_parent(&pipe, child_buf + bytes_written, bytes_read - bytes_written);
                if (res <= 0) break;
                bytes_written += res;
            }
        }

        free(child_buf);
        pipe.close_all (&pipe);
        return 0;

    } else {
        //parent
        pipe.prepare_parent(&pipe);

        int input_fd = open(argv[1], O_RDONLY);
        if (input_fd < 0) {
            perror("Ошибка открытия входного файла");
            pipe.close_all(&pipe);
            return 1;
        }

        int output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            perror("Ошибка создания выходного файла");
            close(input_fd);
            pipe.close_all(&pipe);
            return 1;
        }

        // Защита от deadlock а
        int flags = fcntl(pipe.c2p[0], F_GETFL, 0);
        fcntl(pipe.c2p[0], F_SETFL, flags | O_NONBLOCK);

        char* write_buf = malloc(BUFFER_SIZE);
        char* read_buf = malloc(BUFFER_SIZE);
        
        size_t write_pos = 0, write_len = 0;
        int file_eof = 0;

        while (!file_eof || write_len > 0) {
            if (write_len == 0 && !file_eof) {
                ssize_t bytes_from_file = read(input_fd, write_buf, BUFFER_SIZE);
                if (bytes_from_file < 0) {
                    perror("Ошибка чтения файла");
                    break;
                }
                if (bytes_from_file == 0) {
                    file_eof = 1;
                    close(pipe.p2c[1]);
                } else {
                    write_len = bytes_from_file;
                    write_pos = 0;
                }
            }

            if (write_len > 0) {
                ssize_t bytes_to_child = pipe.write_to_child(&pipe, write_buf + write_pos, write_len);
                if (bytes_to_child > 0) {
                    write_pos += bytes_to_child;
                    write_len -= bytes_to_child;
                }
            }

            ssize_t bytes_from_child;
            while ((bytes_from_child = pipe.read_from_child(&pipe, read_buf, BUFFER_SIZE)) > 0) {
                write(output_fd, read_buf, bytes_from_child);
            }
        }

        fcntl(pipe.c2p[0], F_SETFL, flags);
        ssize_t bytes_from_child;
        while ((bytes_from_child = pipe.read_from_child(&pipe, read_buf, BUFFER_SIZE)) > 0) {
            write(output_fd, read_buf, bytes_from_child);
        }

        free(write_buf);
        free(read_buf);
        close(input_fd);
        close(output_fd);
        close(pipe.c2p[0]);

        wait(NULL);
    }

    return 0;
}
