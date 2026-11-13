#ifndef DUPLEX_H
#define DUPLEX_H

#include <stddef.h>
#include <sys/types.h>

typedef struct DuplexPipe DuplexPipe;

typedef struct {
    ssize_t (*receive)(DuplexPipe *self, void *buffer, size_t size);
    int (*send)(DuplexPipe *self, const void *buffer, size_t size);
    void (*finish)(DuplexPipe *self);
    void (*close)(DuplexPipe *self);
} DuplexMethods;

struct DuplexPipe {
    int read_fd;
    int write_fd;
    const DuplexMethods *methods;
};

int duplex_pair(DuplexPipe endpoints[2]);

#endif
