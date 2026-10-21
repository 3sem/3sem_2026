#include "transfer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

struct Channel {
    int fifo_fds[2];
};

const char *channelName(void) {
    return "fifo";
}

Channel *channelCreate(size_t buffer_size) {
    (void) buffer_size;
    Channel *channel = malloc(sizeof(*channel));
    if (channel == NULL) {
        return NULL;
    }
    if (openFifo(channel->fifo_fds) < 0) {
        free(channel);
        return NULL;
    }
    return channel;
}

int channelSelectSide(Channel *channel, bool sender) {
    int unused_side = sender ? 0 : 1;
    int fd = channel->fifo_fds[1 - unused_side];
    if (close(channel->fifo_fds[unused_side]) < 0) {
        return -1;
    }
    channel->fifo_fds[unused_side] = -1;
    int flags = fcntl(fd, F_GETFL);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int channelSend(Channel *channel, const unsigned char *data, size_t size) {
    if (size == 0) {
        int result = close(channel->fifo_fds[1]);
        channel->fifo_fds[1] = -1;
        return result;
    }
    return writeAll(channel->fifo_fds[1], data, size);
}

ssize_t channelReceive(Channel *channel, unsigned char *data, size_t capacity) {
    return readBlock(channel->fifo_fds[0], data, capacity);
}

int channelDestroy(Channel *channel) {
    int result = 0;
    for (size_t index = 0; index < 2; ++index) {
        if (channel->fifo_fds[index] >= 0 && close(channel->fifo_fds[index]) < 0) {
            result = -1;
        }
    }
    free(channel);
    return result;
}
