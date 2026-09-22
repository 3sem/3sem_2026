#include "dup_pipe.h"

int emulation() {
    Channel channel;
    if (channel_init(&channel) == -1) {
        perror("init_error\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork_err\n");
        return -1;
    }

    //Parent
    if (pid > 0) {
        if (channel.ops.close_unused(&channel) == -1) {
            return -1;
        }
       int file_fd = open("txt/input.txt", O_RDONLY);
       int out_fd = open("txt/output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (file_fd == -1 || out_fd == -1) {
            perror("open");
            return 1;
        }
        
        char buffer[BUF_SZ];
        ssize_t n;
        
        while ((n = read(file_fd, buffer, BUF_SZ)) > 0) {
            if (channel.ops.send(&channel, buffer, n) == -1) {
                perror("send failed\n");
                close(file_fd);
                close(out_fd);
                return -1;
            }
            int received = channel.ops.receive(&channel, buffer, BUF_SZ);
            if (received == -1) {
                perror("receive failed\n");
                close(file_fd);
                close(out_fd);
                return -1;
            }

            ssize_t total = 0;

            while (total < received) {
                ssize_t written = write(out_fd, buffer + total, received - total);
                if (written == -1) {
                    perror("write\n");
                }
                total += written;
            }
        }
        
        close(file_fd);
        close(out_fd);
        
        if (channel.ops.close_end(&channel) == -1) {
            perror("close_end failed\n");
            return -1;
        }
        
        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
            return -1;
        } 
    }
    //Child
    else {
        channel.is_parent = false;

        if (channel.ops.close_unused(&channel) == -1) {
            return 1;
        }
        char buffer[BUF_SZ];

        while(true) {
            int n = channel.ops.receive(&channel, buffer, BUF_SZ);

            if (n == -1) {
                perror("receive\n");
                return -1;
            }

            if (n == 0) {
                break;
            }

            if (channel.ops.send(&channel, buffer, n) == -1) {
                return -1;
            }
        }

        if (channel.ops.close_end(&channel) == -1) {
            perror("close_end failed\n");
            return 1;
        }
    }
    return 0;
}


int channel_init(Channel* channel) {
    if (channel == NULL) {
        perror("incorrect channel\n");
        return -1;
    }

    if (pipe(channel->parent_to_child) == -1) {
        perror("pipe err\n");
        return -1;
    }

    if (pipe(channel->child_to_parent) == -1) {
        close(channel->parent_to_child[0]);
        close(channel->parent_to_child[1]);
        perror("pipe err\n");
        return -1;
    }

    channel->ops.send = chann_send;
    channel->ops.receive = chann_receive;
    channel->ops.close_end = close_end;
    channel->ops.close_unused = close_unused;
    
    channel->is_parent = true;

    return 0;
}

int chann_send(Channel* self, void* buffer, size_t size) {
    int fd = 0;
    size_t total = 0;
    if (self->is_parent) {
        fd = self->parent_to_child[1];
    }
    else {
        fd = self->child_to_parent[1];
    }

    while (total < size) {
        ssize_t n = write(fd, (const char*)buffer + total, size - total);
        if (n == -1) {
            return -1;
        }
        total += n;
    }

    return total;
}

int chann_receive(Channel* self, void* buffer, size_t size) {
    int fd = 0;

    if (self->is_parent) {
        fd = self->child_to_parent[0];
    }
    else {
        fd = self->parent_to_child[0];
    }

    return read(fd, buffer, size);
}


int close_unused(Channel* self) {
    if (self == NULL) {
        perror("input error\n");
        return -1;
    }

    if (self->is_parent) {
        if (close(self->parent_to_child[0]) == -1 ||
            close(self->child_to_parent[1]) == -1) 
        {
            perror("close error(unused)\n");
            return -1;
        }
    } else {
        if (close(self->parent_to_child[1]) == -1 ||
            close(self->child_to_parent[0]) == -1) 
        {
            perror("close error(unused)\n");
            return -1;
        }
    }

    return 0;
}

int close_end(Channel *self)
{
    if (self == NULL)
        return -1;

    int result = 0;

    if (self->is_parent) {
        if (close(self->parent_to_child[1]) == -1)
            result = -1;

        if (close(self->child_to_parent[0]) == -1)
            result = -1;
    }
    else {
        if (close(self->parent_to_child[0]) == -1)
            result = -1;

        if (close(self->child_to_parent[1]) == -1)
            result = -1;
    }

    return result;
}

