#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHUNK 4096

typedef struct duplex Duplex;
typedef struct duplex_ops Ops;

struct duplex_ops {
    int (*push)          (Duplex *self, char *data, int len);
    int (*pull)          (Duplex *self, char *data, int len);
    int (*shutdown_side) (Duplex *self);
    int (*shutdown_all)  (Duplex *self);
};

struct duplex {
    int upstream[2];
    int downstream[2];
    int is_parent;
    Ops ops;
};

int dpx_push(Duplex *self, char *data, int len) {
    int fd = self->is_parent ? self->upstream[1] : self->downstream[1];
    int sent = 0;

    while (sent < len) {
        int n = write(fd, data + sent, len - sent);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("write");
            return -1;
        }
        sent += n;
    }
    return sent;
}

int dpx_pull(Duplex *self, char *data, int len) {
    int fd = self->is_parent ? self->downstream[0] : self->upstream[0];
    int n;

    do {
        n = read(fd, data, len);
    } while (n == -1 && errno == EINTR);

    if (n == -1) {
        perror("read");
        return -1;
    }
    return n;
}

int dpx_shutdown_side(Duplex *self) {
    if (self->is_parent) {
        if (close(self->upstream[0]) == -1) {
            perror("close");
            return -1;
        }
        if (close(self->downstream[1]) == -1) {
            perror("close");
            return -1;
        }
    } else {
        if (close(self->upstream[1]) == -1) {
            perror("close");
            return -1;
        }
        if (close(self->downstream[0]) == -1) {
            perror("close");
            return -1;
        }
    }
    return 0;
}

int dpx_shutdown_all(Duplex *self) {
    if (self->is_parent) {
        close(self->upstream[1]);
        close(self->downstream[0]);
    } else {
        close(self->upstream[0]);
        close(self->downstream[1]);
    }
    return 0;
}

int dpx_init(Duplex *d) {
    if (pipe(d->upstream) == -1) {
        perror("pipe");
        return -1;
    }
    if (pipe(d->downstream) == -1) {
        close(d->upstream[0]);
        close(d->upstream[1]);
        perror("pipe");
        return -1;
    }

    d->ops.push          = dpx_push;
    d->ops.pull          = dpx_pull;
    d->ops.shutdown_side = dpx_shutdown_side;
    d->ops.shutdown_all  = dpx_shutdown_all;
    d->is_parent         = 1;

    return 0;
}

int copy_via_child(void) {
    Duplex chan;

    if (dpx_init(&chan) == -1) {
        printf("init_error\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        printf("fork_err\n");
        return -1;
    }

    if (pid > 0) {
        if (chan.ops.shutdown_side(&chan) == -1) {
            waitpid(pid, NULL, 0);
            return -1;
        }

        int src = open("input.txt", O_RDONLY);
        int dst = open("output.txt",
                       O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (src == -1 || dst == -1) {
            perror("open");
            if (src != -1) close(src);
            if (dst != -1) close(dst);
            chan.ops.shutdown_all(&chan);
            waitpid(pid, NULL, 0);
            return -1;
        }

        char buf[CHUNK];
        int n;

        while ((n = read(src, buf, CHUNK)) > 0) {
            if (chan.ops.push(&chan, buf, n) == -1) {
                printf("push failed\n");
                close(src);
                close(dst);
                chan.ops.shutdown_all(&chan);
                waitpid(pid, NULL, 0);
                return -1;
            }

            int got = 0;
            while (got < n) {
                int r = chan.ops.pull(&chan, buf + got, n - got);
                if (r <= 0)
                    break;
                got += r;
            }

            if (got < n) {
                printf("pull failed\n");
                close(src);
                close(dst);
                chan.ops.shutdown_all(&chan);
                waitpid(pid, NULL, 0);
                return -1;
            }

            int done = 0;
            while (done < got) {
                int w = write(dst, buf + done, got - done);
                if (w == -1) {
                    perror("write");
                    close(src);
                    close(dst);
                    chan.ops.shutdown_all(&chan);
                    waitpid(pid, NULL, 0);
                    return -1;
                }
                done += w;
            }
        }

        close(src);
        close(dst);

        if (chan.ops.shutdown_all(&chan) == -1) {
            printf("shutdown_all failed\n");
            waitpid(pid, NULL, 0);
            return -1;
        }

        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
            return -1;
        }
    }
    else {
        chan.is_parent = 0;

        if (chan.ops.shutdown_side(&chan) == -1) {
            chan.ops.shutdown_all(&chan);
            return -1;
        }

        char buf[CHUNK];

        while (1) {
            int n = chan.ops.pull(&chan, buf, CHUNK);

            if (n == -1) {
                printf("pull\n");
                chan.ops.shutdown_all(&chan);
                return -1;
            }
            if (n == 0)
                break;

            if (chan.ops.push(&chan, buf, n) == -1) {
                chan.ops.shutdown_all(&chan);
                return -1;
            }
        }

        if (chan.ops.shutdown_all(&chan) == -1) {
            printf("shutdown_all failed\n");
            return -1;
        }
    }

    return 0;
}

int main(void) {
    return copy_via_child();
}
