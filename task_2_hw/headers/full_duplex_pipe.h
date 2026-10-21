#ifndef FULL_DUPLEX_PIPE_H
#define FULL_DUPLEX_PIPE_H

#include <stddef.h>
#include <sys/types.h>

typedef struct FullDuplexPipe FullDuplexPipe;

typedef enum PipeSide {
    PIPE_PARENT,
    PIPE_CHILD
} PipeSide;
// Object Orientir style in C
typedef struct PipeOps {
    int (*selectSide)(FullDuplexPipe *self, PipeSide side);
    ssize_t (*send)(FullDuplexPipe *self, const void *buffer, size_t size);
    ssize_t (*receive)(FullDuplexPipe *self, void *buffer, size_t size);
    void (*closeWrite)(FullDuplexPipe *self);
    void (*destroy)(FullDuplexPipe *self);
} PipeOps;

struct FullDuplexPipe {
    int parentToChild[2];
    int childToParent[2];
    int readFd;
    int writeFd;
    int sideSelected;
    PipeOps actions;
};

int fullDuplexPipeInit(FullDuplexPipe *self);

#endif // FULL_DUPLEX_PIPE_H
