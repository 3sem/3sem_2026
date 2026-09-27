#define _GNU_SOURCE

#include "duplex_pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static ssize_t duplex_read(DuplexPipe *self, void *buf, size_t n) {
  for (;;) {
    ssize_t got = read(self->read_fd, buf, n);
    if (got < 0 && errno == EINTR)
      continue;
    return got;
  }
}

static ssize_t duplex_write(DuplexPipe *self, const void *buf, size_t n) {
  for (;;) {
    ssize_t put = write(self->write_fd, buf, n);
    if (put < 0 && errno == EINTR)
      continue;
    return put;
  }
}

static ssize_t splice_exact(int in_fd, int out_fd, size_t n) {
  size_t done = 0;

  while (done < n) {
    ssize_t moved = splice(in_fd, NULL, out_fd, NULL, n - done, SPLICE_F_MORE);
    if (moved < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (moved == 0)
      return -1;
    done += (size_t)moved;
  }
  return (ssize_t)done;
}

static ssize_t duplex_splice_from(DuplexPipe *self, int in_fd, size_t n) {
  return splice_exact(in_fd, self->write_fd, n);
}

static ssize_t duplex_splice_to(DuplexPipe *self, int out_fd, size_t n) {
  return splice_exact(self->read_fd, out_fd, n);
}

static ssize_t duplex_relay(DuplexPipe *self, size_t n) {
  for (;;) {
    ssize_t moved =
        splice(self->read_fd, NULL, self->write_fd, NULL, n, SPLICE_F_MORE);
    if (moved < 0 && errno == EINTR)
      continue;
    return moved;
  }
}

static void duplex_close_write(DuplexPipe *self) {
  if (self->write_fd >= 0) {
    close(self->write_fd);
    self->write_fd = -1;
  }
}

static void duplex_destroy(DuplexPipe *self) {
  if (self == NULL)
    return;
  if (self->read_fd >= 0)
    close(self->read_fd);
  if (self->write_fd >= 0)
    close(self->write_fd);
  free(self);
}

static int set_pipe_capacity(int fd) {
  static const int want[] = {16 * 1024 * 1024, 8 * 1024 * 1024, 4 * 1024 * 1024,
                             1024 * 1024,      256 * 1024,      64 * 1024};

  for (size_t i = 0; i < sizeof want / sizeof want[0]; i++) {
    int got = fcntl(fd, F_SETPIPE_SZ, want[i]);
    if (got > 0)
      return got;
  }
  return 64 * 1024;
}

static void close_pair(const int fd[2]) {
  if (fd[0] >= 0)
    close(fd[0]);
  if (fd[1] >= 0)
    close(fd[1]);
}

DuplexPipe *constructDuplexPipe(pid_t *child_pid) {
  int to_child[2] = {-1, -1};
  int to_parent[2] = {-1, -1};
  DuplexPipe *self;
  int cap_forward;
  int cap_back;
  pid_t pid;

  if (pipe(to_child) < 0)
    return NULL;
  if (pipe(to_parent) < 0) {
    close_pair(to_child);
    return NULL;
  }

  cap_forward = set_pipe_capacity(to_child[1]);
  cap_back = set_pipe_capacity(to_parent[1]);

  self = calloc(1, sizeof(*self));
  if (self == NULL) {
    int saved = errno;
    close_pair(to_child);
    close_pair(to_parent);
    errno = saved; /* close() имеет право затереть ENOMEM */
    return NULL;
  }

  self->read_fd = -1;
  self->write_fd = -1;
  // Кусок передачи не может быть больше меньшей из двух труб
  self->capacity = cap_forward < cap_back ? cap_forward : cap_back;
  self->actions.read = duplex_read;
  self->actions.write = duplex_write;
  self->actions.splice_from = duplex_splice_from;
  self->actions.splice_to = duplex_splice_to;
  self->actions.relay = duplex_relay;
  self->actions.close_write = duplex_close_write;
  self->actions.destroy = duplex_destroy;

  pid = fork();
  if (pid < 0) {
    int saved = errno;
    close_pair(to_child);
    close_pair(to_parent);
    free(self);
    errno = saved;
    return NULL;
  }

  if (pid == 0) {
    close(to_child[1]);
    close(to_parent[0]);
    self->read_fd = to_child[0];
    self->write_fd = to_parent[1];
    if (child_pid != NULL)
      *child_pid = 0;
  } else {
    close(to_child[0]);
    close(to_parent[1]);
    self->read_fd = to_parent[0];
    self->write_fd = to_child[1];
    if (child_pid != NULL)
      *child_pid = pid;
  }

  return self;
}

int duplex_read_n(DuplexPipe *self, void *buf, size_t n) {
  char *dst = buf;
  size_t got = 0;

  while (got < n) {
    ssize_t part = self->actions.read(self, dst + got, n - got);
    if (part < 0)
      return -1;
    if (part == 0)
      return -1;
    got += (size_t)part;
  }
  return 0;
}

int duplex_write_n(DuplexPipe *self, const void *buf, size_t n) {
  const char *src = buf;
  size_t put = 0;

  while (put < n) {
    ssize_t part = self->actions.write(self, src + put, n - put);
    if (part < 0)
      return -1;
    if (part == 0)
      return -1;
    put += (size_t)part;
  }
  return 0;
}
