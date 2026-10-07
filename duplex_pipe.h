#ifndef DUPLEX_PIPE_H
#define DUPLEX_PIPE_H

#include <stddef.h>
#include <sys/types.h>

typedef struct duplex_pipe DuplexPipe;
typedef struct duplex_actions DuplexActions;

struct duplex_actions {
  ssize_t (*read)(DuplexPipe *self, void *buf, size_t n);
  ssize_t (*write)(DuplexPipe *self, const void *buf, size_t n);

  ssize_t (*splice_from)(DuplexPipe *self, int in_fd, size_t n);
  ssize_t (*splice_to)(DuplexPipe *self, int out_fd, size_t n);

  ssize_t (*relay)(DuplexPipe *self, size_t n);

  void (*close_write)(DuplexPipe *self);
  void (*destroy)(DuplexPipe *self);
};

struct duplex_pipe {
  int read_fd;
  int write_fd;
  int capacity; // фактический размер буфера каждой трубы, в байтах
  DuplexActions actions;
};

DuplexPipe *constructDuplexPipe(pid_t *child_pid);

int duplex_read_n(DuplexPipe *self, void *buf, size_t n);
int duplex_write_n(DuplexPipe *self, const void *buf, size_t n);

#endif
