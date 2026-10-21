#include "transfer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct Message {
    long type;
    unsigned char data[];
} Message;

struct Channel {
    int queue_id;
    size_t message_limit;
    bool eof;
    Message *message;
};

const char *channelName(void) {
    return "mq";
}

int channelDestroy(Channel *channel) {
    int result = 0;
    if (channel->queue_id >= 0 && msgctl(channel->queue_id, IPC_RMID, NULL) < 0) {
        result = -1;
    }
    free(channel->message);
    free(channel);
    return result;
}

Channel *channelCreate(size_t buffer_size) {
    Channel *channel = calloc(1, sizeof(*channel));
    if (channel == NULL) {
        return NULL;
    }
    channel->queue_id = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (channel->queue_id < 0) {
        goto fail;
    }
    channel->message_limit = probeMessageLimit(channel->queue_id);
    if (channel->message_limit == 0) {
        goto fail;
    }
    if (channel->message_limit > buffer_size) {
        channel->message_limit = buffer_size;
    }
    channel->message = malloc(sizeof(Message) + channel->message_limit);
    if (channel->message == NULL) {
        goto fail;
    }
    channel->message->type = 1;
    return channel;

fail: {
        int saved_errno = errno;
        channelDestroy(channel);
        errno = saved_errno;
        return NULL;
    }
}

int channelSelectSide(Channel *channel, bool sender) {
    (void) channel;
    (void) sender;
    return 0;
}

int channelSend(Channel *channel, const unsigned char *data, size_t size) {
    size_t offset = 0;
    /* Execute once even for EOF: a zero-length SYS V message is valid. */
    do {
        size_t count = size - offset;
        if (count > channel->message_limit) {
            count = channel->message_limit;
        }
        memcpy(channel->message->data, data + offset, count);
        for (;;) {
            if (shouldStop()) {
                return -1;
            }
            if (msgsnd(channel->queue_id, channel->message, count, 0) == 0) {
                break;
            }
            if (errno != EINTR) {
                return -1;
            }
        }
        offset += count;
    } while (offset < size);
    return 0;
}

ssize_t channelReceive(Channel *channel, unsigned char *data, size_t capacity) {
    size_t offset = 0;
    while (offset < capacity && !channel->eof) {
        if (shouldStop()) {
            return -1;
        }
        ssize_t count = msgrcv(channel->queue_id, channel->message,
                              channel->message_limit, 1, 0);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if ((size_t) count > capacity - offset) {
            errno = EMSGSIZE;
            return -1;
        }
        if (count == 0) {
            channel->eof = true;
            break;
        }
        memcpy(data + offset, channel->message->data, (size_t) count);
        offset += (size_t) count;
    }
    return (ssize_t) offset;
}
