#include "duplex.h"

#include <errno.h>
#include <unistd.h>

static ssize_t receive_data(DuplexPipe *self, void *buffer, size_t size)
{
    ssize_t received;
    do {
        received = read(self->read_fd, buffer, size);
    } while (received < 0 && errno == EINTR);
    return received;
}

static int send_data(DuplexPipe *self, const void *buffer, size_t size)
{
    const char *position = buffer;
    while (size > 0) {
        ssize_t sent = write(self->write_fd, position, size);
        if (sent < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (sent == 0) {
            errno = EIO;
            return -1;
        }
        position += sent;
        size -= (size_t)sent;
    }
    return 0;
}

static void finish_sending(DuplexPipe *self)
{
    if (self->write_fd >= 0) {
        close(self->write_fd);
        self->write_fd = -1;
    }
}

static void close_endpoint(DuplexPipe *self)
{
    finish_sending(self);
    if (self->read_fd >= 0) {
        close(self->read_fd);
        self->read_fd = -1;
    }
}

static const DuplexMethods methods = {
    receive_data, send_data, finish_sending, close_endpoint
};

int duplex_pair(DuplexPipe endpoints[2])
{
    int forward[2];
    int backward[2];
    if (pipe(forward) < 0)
        return -1;
    if (pipe(backward) < 0) {
        int saved_errno = errno;
        close(forward[0]);
        close(forward[1]);
        errno = saved_errno;
        return -1;
    }
    endpoints[0] = (DuplexPipe){backward[0], forward[1], &methods};
    endpoints[1] = (DuplexPipe){forward[0], backward[1], &methods};
    return 0;
}
