#include "transfer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

typedef struct SharedBlock {
    size_t size;
    unsigned char data[];
} SharedBlock;

union SemArgument {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct Channel {
    int shm_id;
    int sem_id;
    size_t capacity;
    SharedBlock *block;
};

enum { EMPTY_SLOT, FULL_SLOT };

const char *channelName(void) {
    return "shm";
}

int channelDestroy(Channel *channel) {
    int result = 0;
    if (channel->block != NULL && shmdt(channel->block) < 0) {
        result = -1;
    }
    if (channel->shm_id >= 0 && shmctl(channel->shm_id, IPC_RMID, NULL) < 0) {
        result = -1;
    }
    if (channel->sem_id >= 0 && semctl(channel->sem_id, 0, IPC_RMID) < 0) {
        result = -1;
    }
    free(channel);
    return result;
}

Channel *channelCreate(size_t buffer_size) {
    Channel *channel = calloc(1, sizeof(*channel));
    if (channel == NULL) {
        return NULL;
    }
    channel->shm_id = -1;
    channel->sem_id = -1;
    channel->capacity = buffer_size;
    channel->shm_id = shmget(IPC_PRIVATE, sizeof(SharedBlock) + buffer_size,
                             IPC_CREAT | 0600);
    if (channel->shm_id < 0) {
        goto fail;
    }
    void *address = shmat(channel->shm_id, NULL, 0);
    if (address == (void *) -1) {
        goto fail;
    }
    channel->block = address;
    channel->sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (channel->sem_id < 0) {
        goto fail;
    }
    unsigned short initial_values[] = {1, 0};
    union SemArgument argument = {.array = initial_values};
    if (semctl(channel->sem_id, 0, SETALL, argument) < 0) {
        goto fail;
    }
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

static int changeSemaphore(Channel *channel, unsigned short index, short change) {
    struct sembuf operation = {.sem_num = index, .sem_op = change, .sem_flg = 0};
    for (;;) {
        if (shouldStop()) {
            return -1;
        }
        if (semop(channel->sem_id, &operation, 1) == 0) {
            return 0;
        }
        if (errno != EINTR) {
            return -1;
        }
    }
}

int channelSend(Channel *channel, const unsigned char *data, size_t size) {
    if (size > channel->capacity) {
        errno = EMSGSIZE;
        return -1;
    }
    if (changeSemaphore(channel, EMPTY_SLOT, -1) < 0) {
        return -1;
    }
    memcpy(channel->block->data, data, size);
    channel->block->size = size;
    return changeSemaphore(channel, FULL_SLOT, 1);
}

ssize_t channelReceive(Channel *channel, unsigned char *data, size_t capacity) {
    if (changeSemaphore(channel, FULL_SLOT, -1) < 0) {
        return -1;
    }
    size_t size = channel->block->size;
    if (size > capacity) {
        errno = EMSGSIZE;
        return -1;
    }
    memcpy(data, channel->block->data, size);
    if (changeSemaphore(channel, EMPTY_SLOT, 1) < 0) {
        return -1;
    }
    return (ssize_t) size;
}
