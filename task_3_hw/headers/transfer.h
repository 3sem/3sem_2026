#ifndef TRANSFER_H
#define TRANSFER_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_BUFFER_SIZE (16U * 1024U * 1024U)

typedef struct Channel Channel;

/* Each executable links exactly one implementation of this interface. */
const char *channelName(void);
Channel *channelCreate(size_t buffer_size);
int channelSelectSide(Channel *channel, bool sender);
int channelSend(Channel *channel, const unsigned char *data, size_t size);
ssize_t channelReceive(Channel *channel, unsigned char *data, size_t capacity);
int channelDestroy(Channel *channel);

int shouldStop(void);
ssize_t readBlock(int fd, void *buffer, size_t size);
int writeAll(int fd, const void *buffer, size_t size);

/* Probes use private, temporary IPC resources and run before timing. */
size_t probeMessageLimit(int queue_id);
long probeFifoCapacity(void);
int openFifo(int fifo_fds[2]);

#endif
