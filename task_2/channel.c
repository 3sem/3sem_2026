#define _POSIX_C_SOURCE 200809L
#include "channel.h"

#include <errno.h>
#include <unistd.h>

static int write_all(int fd, const void *data, size_t size)
{
    const char *p = data;
    size_t done = 0;

    while (done < size) {
        ssize_t n = write(fd, p + done, size - done);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        if (n == 0)
            return -1;

        done += (size_t)n;
    }

    return 0;
}

static int send_data(Channel *ch, const void *data, size_t size)
{
    return write_all(ch->write_fd, data, size);
}

static ssize_t receive_data(Channel *ch, void *data, size_t size)
{
    ssize_t n;

    do {
        n = read(ch->read_fd, data, size);
    } while (n < 0 && errno == EINTR);

    return n;
}

static void close_send(Channel *ch)
{
    if (ch->write_fd != -1) {
        close(ch->write_fd);
        ch->write_fd = -1;
    }
}

static void close_all(Channel *ch)
{
    close_send(ch);

    if (ch->read_fd != -1) {
        close(ch->read_fd);
        ch->read_fd = -1;
    }
}

static const ChannelOps actions = {
    send_data,
    receive_data,
    close_send,
    close_all
};

int channel_make(Channel *parent, Channel *child)
{
    int to_child[2];
    int to_parent[2];

    if (pipe(to_child) == -1)
        return -1;

    if (pipe(to_parent) == -1) {
        close(to_child[0]);
        close(to_child[1]);
        return -1;
    }

    parent->read_fd = to_parent[0];
    parent->write_fd = to_child[1];
    parent->ops = &actions;

    child->read_fd = to_child[0];
    child->write_fd = to_parent[1];
    child->ops = &actions;

    return 0;
}
