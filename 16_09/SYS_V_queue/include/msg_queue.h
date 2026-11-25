#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

#define QUEUE_MESSAGE_TYPE 1L

struct QueueMessage {
    long mtype;
    size_t size;
    bool finished;
    char data[];
};

#define QUEUE_MESSAGE_METADATA_SIZE \
    (offsetof(struct QueueMessage, data) - sizeof(long))

#endif
