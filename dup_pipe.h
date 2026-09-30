#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SZ 4096

typedef struct channel Channel;
typedef struct channel_ops Ops;

struct channel_ops {
    int (*send)         (Channel *self, char *buf, int size);
    int (*receive)      (Channel *self, char *buf, int size);
    int (*close_unused) (Channel *self);
    int (*close_end)    (Channel *self);
};

struct channel {
    int parent_to_child[2];
    int child_to_parent[2];
    int is_parent;
    Ops ops;
};

int channel_init(Channel *channel);
int pipe_work();