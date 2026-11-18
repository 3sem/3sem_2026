#ifndef CHANNEL_H
#define CHANNEL_H

#include <stddef.h>
#include <sys/types.h>

typedef struct Channel Channel;
typedef struct ChannelOps ChannelOps;

struct ChannelOps {
    int (*send)(Channel *ch, const void *data, size_t size);
    ssize_t (*receive)(Channel *ch, void *data, size_t size);
    void (*close_send)(Channel *ch);
    void (*close_all)(Channel *ch);
};

struct Channel {
    int read_fd;
    int write_fd;
    const ChannelOps *ops;
};

int channel_make(Channel *parent, Channel *child);

#endif
