#define _GNU_SOURCE
#include "duplex.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#ifndef F_SETPIPE_SZ
#define F_SETPIPE_SZ 1031
#endif
#ifndef F_GETPIPE_SZ
#define F_GETPIPE_SZ 1032
#endif


static ssize_t duplex_send(Duplex* self, const void* buf, size_t len)
{
    const char* p = (const char*)buf;
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(self->out, p + total, len - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            return total ? (ssize_t)total : -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static ssize_t duplex_recv(Duplex* self, void* buf, size_t len)
{
    char* p = (char*)buf;
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(self->in, p + total, len - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            return total ? (ssize_t)total : -1;
        }
        if (n == 0) break;            
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static ssize_t duplex_sendfile(Duplex* self, int in_fd, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = splice(in_fd, NULL, self->out, NULL,
                           len - total, SPLICE_F_MOVE);
        if (n < 0) {
            if (errno == EINTR) continue;
            return total ? (ssize_t)total : -1;
        }
        if (n == 0) break;            
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static ssize_t duplex_recvfile(Duplex* self, int out_fd, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = splice(self->in, NULL, out_fd, NULL,
                           len - total, SPLICE_F_MOVE);
        if (n < 0) {
            if (errno == EINTR) continue;
            return total ? (ssize_t)total : -1;
        }
        if (n == 0) break;            
        total += (size_t)n;
    }
    return (ssize_t)total;
}

static ssize_t duplex_echo(Duplex* self, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t n = splice(self->in, NULL, self->out, NULL,
                           len - total, SPLICE_F_MOVE);
        if (n < 0) {
            if (errno == EINTR) continue;
            return total ? (ssize_t)total : -1;
        }
        if (n == 0) break;
        total += (size_t)n;
    }
    return (ssize_t)total;
}

Pair Construct(size_t pipe_size)
{
    Pair p;
    Duplex* d1 = (Duplex*)calloc(1, sizeof(Duplex));
    Duplex* d2 = (Duplex*)calloc(1, sizeof(Duplex));
    if (!d1 || !d2) { perror("calloc"); exit(EXIT_FAILURE); }

    /* общая таблица методов */
    d1->op_table.Send     = duplex_send;
    d1->op_table.Recv     = duplex_recv;
    d1->op_table.SendFile = duplex_sendfile;
    d1->op_table.RecvFile = duplex_recvfile;
    d1->op_table.Echo     = duplex_echo;
    d2->op_table          = d1->op_table;

    int pipes[2][2];
    if (pipe(pipes[0]) < 0 || pipe(pipes[1]) < 0) {
        perror("pipe"); exit(EXIT_FAILURE);
    }

    if (pipe_size > 0) {
        fcntl(pipes[0][0], F_SETPIPE_SZ, (int)pipe_size);
        fcntl(pipes[0][1], F_SETPIPE_SZ, (int)pipe_size);
        fcntl(pipes[1][0], F_SETPIPE_SZ, (int)pipe_size);
        fcntl(pipes[1][1], F_SETPIPE_SZ, (int)pipe_size);
    }

    int sz = fcntl(pipes[0][0], F_GETPIPE_SZ);
    d1->pipe_size = d2->pipe_size = (sz > 0) ? (size_t)sz : 65536;

    d1->in  = pipes[0][0];
    d1->out = pipes[1][1];

    d2->in  = pipes[1][0];
    d2->out = pipes[0][1];

    p.duplex_1 = d1;
    p.duplex_2 = d2;
    return p;
}

void Destruct(Duplex* self)
{
    if (!self) return;
    if (self->in  >= 0) close(self->in);
    if (self->out >= 0) close(self->out);
    free(self);
}