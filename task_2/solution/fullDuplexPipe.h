#ifndef FULL_DUPLEX_PIPE_H
#define FULL_DUPLEX_PIPE_H

#include "sys/types.h"

typedef struct op_table Ops;

struct fDupPipe_t;

typedef struct op_table{
    pid_t (*fork)(fDupPipe_t *self);

    ssize_t (*writeDown)(struct fDupPipe_t* fDupPipe, const void* buf, size_t count);                        
    ssize_t (*writeUp)(struct fDupPipe_t* fDupPipe, const void* buf, size_t count);                        

    ssize_t (*readDown)(struct fDupPipe_t* fDupPipe, void* buf, size_t count);
    ssize_t (*readUp)(struct fDupPipe_t* fDupPipe, void* buf, size_t count);
} Ops;

struct fDupPipe_t{
    int pipeDown[2];
    int pipeUp[2];

    bool isForked;
    pid_t childPid;

    Ops operations;
};

struct fDupPipe_t* fDupPipeCtor();
void               fDupPipeDtor(struct fDupPipe_t* fDupPipe);

pid_t fDupPipeFork(struct fDupPipe_t* fDupPipe);

ssize_t fDupPipeWriteDown(struct fDupPipe_t* fDupPipe, const void* buf, size_t count);
ssize_t fDupPipeWriteUp(struct fDupPipe_t* fDupPipe, const void* buf, size_t count);

ssize_t fDupPipeReadDown(struct fDupPipe_t* fDupPipe, void* buf, size_t count);
ssize_t fDupPipeReadUp(struct fDupPipe_t* fDupPipe, void* buf, size_t count);

#endif /* FULL_DUPLEX_PIPE_H */
