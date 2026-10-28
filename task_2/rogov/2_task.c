#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUF_SIZE (64 * 1024)

typedef struct FDPipe FDPipe;

typedef struct FDPipeOps {
    void (*close_unused)(FDPipe* self, int is_child);

    ssize_t (*parent_write)(FDPipe* self, const void* buf, size_t count);
    ssize_t (*parent_read)(FDPipe* self, void* buf, size_t count);

    ssize_t (*child_write)(FDPipe* self, const void* buf, size_t count);
    ssize_t (*child_read)(FDPipe* self, void* buf, size_t count);

    int (*destroy)(FDPipe* self);
} FDPipeOps;

struct FDPipe {
    int pipe_p2c[2]; //parent to child; 0 — чтение потомком,  1 — запись родителем.
    int pipe_c2p[2]; //child to parent; 0 — чтение родителем, 1 — запись потомком.

    FDPipeOps ops;
};

void fd_pipe_close_unused(FDPipe* self, int is_child) {
    if (is_child == 1) {
        close(self->pipe_p2c[1]);
        close(self->pipe_c2p[0]);
        self->pipe_p2c[1] = -1;
        self->pipe_c2p[0] = -1;
    }
    else {
        close(self->pipe_p2c[0]);
        close(self->pipe_c2p[1]);
        self->pipe_p2c[0] = -1;
        self->pipe_c2p[1] = -1;
    }
}

//для инкапсуляции

ssize_t fd_pipe_parent_write(FDPipe* self, const void* buf, size_t count) {
    return write(self->pipe_p2c[1], buf, count);
}

ssize_t fd_pipe_parent_read(FDPipe* self, void* buf, size_t count) {
    return read(self->pipe_c2p[0], buf, count);
}

ssize_t fd_pipe_child_write(FDPipe* self, const void* buf, size_t count) {
    return write(self->pipe_c2p[1], buf, count);
}

ssize_t fd_pipe_child_read(FDPipe* self, void* buf, size_t count) {
    return read(self->pipe_p2c[0], buf, count);
}

int fd_pipe_destroy(FDPipe* self) {
    if (self == NULL) {
        return -1;
    }

    for (int i = 0; i < 2; i++) {
        if (self->pipe_p2c[i] != -1) {
            close(self->pipe_p2c[i]);
        }
        if (self->pipe_c2p[i] != -1) {
            close(self->pipe_c2p[i]);
        }
    }

    free(self);

    return 0;
}

FDPipe* create_fd_pipe() {
    FDPipe* self = (FDPipe*) calloc(1, sizeof(FDPipe));
    if (self == NULL) {
        return NULL;
    }

    if (pipe(self->pipe_p2c) < 0 || pipe(self->pipe_c2p) < 0) {
        perror("pipe error");
        free(self);
        return NULL;
    }

    self->ops.close_unused = fd_pipe_close_unused;
    self->ops.parent_write = fd_pipe_parent_write;
    self->ops.parent_read  = fd_pipe_parent_read;
    self->ops.child_write  = fd_pipe_child_write;
    self->ops.child_read   = fd_pipe_child_read;
    self->ops.destroy      = fd_pipe_destroy;

    return self;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Использование: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    FDPipe* channel = create_fd_pipe();
    if (channel == NULL) {
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        return 1;
    }

    if (pid == 0) { //child
        channel->ops.close_unused(channel, 1);
        char* buffer = calloc(sizeof(char), BUF_SIZE);

        ssize_t bytes_read = 0;

        while ((bytes_read = channel->ops.child_read(channel, buffer, BUF_SIZE)) > 0) {
            channel->ops.child_write(channel, buffer, bytes_read);
        }

        free(buffer);
        channel->ops.destroy(channel);
        exit(0);
    }
    else {
        channel->ops.close_unused(channel, 0);

        int in_fd = open(argv[1], O_RDONLY);
        int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);

        char* buf_out = calloc(sizeof(char), BUF_SIZE);
        char* buf_in = calloc(sizeof(char), BUF_SIZE);

        ssize_t bytes_read = 0;
        while ((bytes_read = read(in_fd, buf_out, BUF_SIZE)) > 0) {
            channel->ops.parent_write(channel, buf_out, bytes_read);

            ssize_t echoed = channel->ops.parent_read(channel, buf_in, bytes_read);

            write(out_fd, buf_in, echoed);
        }

        close(in_fd);
        close(out_fd);
        free(buf_out);
        free(buf_in);

        channel->ops.destroy(channel);
        wait(NULL);
    }

    return 0;
}
