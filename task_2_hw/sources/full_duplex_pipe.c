#include "full_duplex_pipe.h"

#include <errno.h>
#include <unistd.h>

static void closeFd(int *fd)
{
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static void pipeCloseWrite(FullDuplexPipe *self)
{
    closeFd(&self->writeFd);
}

static void pipeDestroy(FullDuplexPipe *self)
{
    closeFd(&self->readFd);
    closeFd(&self->writeFd);
    for (int i = 0; i < 2; ++i) {
        closeFd(&self->parentToChild[i]);
        closeFd(&self->childToParent[i]);
    }
}

static int pipeSelectSide(FullDuplexPipe *self, PipeSide side)
{
    if (self->sideSelected || (side != PIPE_PARENT && side != PIPE_CHILD)) {
        errno = EINVAL;
        return -1;
    }
    if (side == PIPE_PARENT) {
        self->readFd = self->childToParent[0];
        self->childToParent[0] = -1;
        self->writeFd = self->parentToChild[1];
        self->parentToChild[1] = -1;
    } else {
        self->readFd = self->parentToChild[0];
        self->parentToChild[0] = -1;
        self->writeFd = self->childToParent[1];
        self->childToParent[1] = -1;
    }
    for (int i = 0; i < 2; ++i) {
        closeFd(&self->parentToChild[i]);
        closeFd(&self->childToParent[i]);
    }
    self->sideSelected = 1;
    return 0;
}

static ssize_t pipeSend(FullDuplexPipe *self, const void *buffer, size_t size)
{
    ssize_t count;
    do {
        count = write(self->writeFd, buffer, size);
    } while (count < 0 && errno == EINTR);
    return count;
}

static ssize_t pipeReceive(FullDuplexPipe *self, void *buffer, size_t size)
{
    ssize_t count;
    do {
        count = read(self->readFd, buffer, size);
    } while (count < 0 && errno == EINTR);
    return count;
}

int fullDuplexPipeInit(FullDuplexPipe *self)
{
    *self = (FullDuplexPipe) {
        .parentToChild = {-1, -1},
        .childToParent = {-1, -1},
        .readFd = -1,
        .writeFd = -1,
        .sideSelected = 0,
        .actions = {pipeSelectSide, pipeSend, pipeReceive,
                    pipeCloseWrite, pipeDestroy}
    };
    if (pipe(self->parentToChild) < 0 || pipe(self->childToParent) < 0) {
        int savedErrno = errno;
        pipeDestroy(self);
        errno = savedErrno;
        return -1;
    }
    return 0;
}
