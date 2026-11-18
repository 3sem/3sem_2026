#include "dup_pipe.h"

int chann_send(Channel *self, char *buf, int size) {
    int fd;

    if (self->is_parent)
        fd = self->parent_to_child[1];
    else
        fd = self->child_to_parent[1];

    int total = 0;
    while (total < size) {
        int n = write(fd, buf + total, size - total);
        if (n == -1) {
            perror("write");
            return -1;
        }
        total += n;
    }

    return total;
}


int chann_receive(Channel *self, char *buf, int size) {
    int fd;

    if (self->is_parent)
        fd = self->child_to_parent[0];
    else
        fd = self->parent_to_child[0];

    int n = read(fd, buf, size);
    if (n == -1) {
        perror("read");
        return -1;
    }

    return n;
}


int close_unused(Channel *self) {
    if (self->is_parent) {
        if (close(self->parent_to_child[0]) == -1) {
            perror("close");
            return -1;
        }
        if (close(self->child_to_parent[1]) == -1) {
            perror("close");
            return -1;
        }
    } else {
        if (close(self->parent_to_child[1]) == -1) {
            perror("close");
            return -1;
        }
        if (close(self->child_to_parent[0]) == -1) {
            perror("close");
            return -1;
        }
    }

    return 0;
}

int close_end(Channel *self) {
    if (self->is_parent) {
        close(self->parent_to_child[1]);
        close(self->child_to_parent[0]);
    } else {
        close(self->parent_to_child[0]);
        close(self->child_to_parent[1]);
    }

    return 0;
}

int channel_init(Channel *channel) {
    if (pipe(channel->parent_to_child) == -1) {
        perror("pipe");
        return -1;
    }

    if (pipe(channel->child_to_parent) == -1) {
        close(channel->parent_to_child[0]);
        close(channel->parent_to_child[1]);
        perror("pipe");
        return -1;
    }

    channel->ops.send = chann_send;
    channel->ops.receive = chann_receive;
    channel->ops.close_unused = close_unused;
    channel->ops.close_end = close_end;

    channel->is_parent = 1;

    return 0;
}

int pipe_work() {
    Channel channel;

    if (channel_init(&channel) == -1) {
        printf("init_error\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        printf("fork_err\n");
        return -1;
    }

    if (pid > 0) {
        if (channel.ops.close_unused(&channel) == -1)
            return -1;

        int file_fd = open("input.txt", O_RDONLY);
        int out_fd  = open("output.txt",
                           O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (file_fd == -1 || out_fd == -1) {
            perror("open");
            return -1;
        }

        char buffer[BUF_SZ];
        int n;

        while ((n = read(file_fd, buffer, BUF_SZ)) > 0) {
            if (channel.ops.send(&channel, buffer, n) == -1) {
                printf("send failed\n");
                close(file_fd);
                close(out_fd);
                return -1;
            }

            int received = channel.ops.receive(&channel, buffer, n);
            if (received == -1) {
                printf("receive failed\n");
                close(file_fd);
                close(out_fd);
                return -1;
            }

            int total = 0;
            while (total < received) {
                int written = write(out_fd, buffer + total,
                                    received - total);
                if (written == -1) {
                    perror("write");
                    return -1;
                }
                total += written;
            }
        }

        close(file_fd);
        close(out_fd);

        if (channel.ops.close_end(&channel) == -1) {
            printf("close_end failed\n");
            return -1;
        }

        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
            return -1;
        }
    }

    else {
        channel.is_parent = 0;

        if (channel.ops.close_unused(&channel) == -1)
            return -1;

        char buffer[BUF_SZ];

        while (1) {
            int n = channel.ops.receive(&channel, buffer, BUF_SZ);

            if (n == -1) {
                printf("receive\n");
                return -1;
            }

            if (n == 0)
                break;

            if (channel.ops.send(&channel, buffer, n) == -1)
                return -1;
        }

        if (channel.ops.close_end(&channel) == -1) {
            printf("close_end failed\n");
            return -1;
        }
    }

    return 0;
}