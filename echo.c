#define _GNU_SOURCE

#include "echo.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int read_full(int fd, void *buf, size_t n) {
  char *dst = buf;
  size_t got = 0;

  while (got < n) {
    ssize_t part = read(fd, dst + got, n - got);
    if (part < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (part == 0)
      return -1;
    got += (size_t)part;
  }
  return 0;
}

static int write_full(int fd, const void *buf, size_t n) {
  const char *src = buf;
  size_t put = 0;

  while (put < n) {
    ssize_t part = write(fd, src + put, n - put);
    if (part < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (part == 0)
      return -1;
    put += (size_t)part;
  }
  return 0;
}

int echo_child(DuplexPipe *dp, EchoMode mode) {
  if (mode == ECHO_MODE_SPLICE) {
    for (;;) {
      ssize_t n = dp->actions.relay(dp, (size_t)dp->capacity);
      if (n == 0)
        return 0;
      if (n < 0) {
        perror("relay");
        return -1;
      }
    }
  }

  size_t chunk = (size_t)dp->capacity;
  char *buf = malloc(chunk);
  if (buf == NULL) {
    perror("malloc");
    return -1;
  }

  for (;;) {
    ssize_t n = dp->actions.read(dp, buf, chunk);
    if (n == 0) {
      free(buf);
      return 0;
    }
    if (n < 0) {
      perror("read");
      free(buf);
      return -1;
    }
    if (duplex_write_n(dp, buf, (size_t)n) < 0) {
      perror("write");
      free(buf);
      return -1;
    }
  }
}

static int transfer(DuplexPipe *dp, int in_fd, int out_fd, off_t size,
                    EchoMode mode) {
  off_t left = size;
  char *buf = NULL;

  if (mode == ECHO_MODE_COPY) {
    buf = malloc((size_t)dp->capacity);
    if (buf == NULL) {
      perror("malloc");
      return -1;
    }
  }

  while (left > 0) {
    size_t chunk = (size_t)dp->capacity;
    if ((off_t)chunk > left)
      chunk = (size_t)left;

    if (mode == ECHO_MODE_SPLICE) {
      if (dp->actions.splice_from(dp, in_fd, chunk) < 0) {
        perror("splice_from");
        free(buf);
        return -1;
      }
      if (dp->actions.splice_to(dp, out_fd, chunk) < 0) {
        perror("splice_to");
        free(buf);
        return -1;
      }
    } else if (read_full(in_fd, buf, chunk) < 0 ||
               duplex_write_n(dp, buf, chunk) < 0 ||
               duplex_read_n(dp, buf, chunk) < 0 ||
               write_full(out_fd, buf, chunk) < 0) {
      perror("transfer");
      free(buf);
      return -1;
    }
    left -= (off_t)chunk;
  }

  free(buf);
  return 0;
}

int echo_parent(DuplexPipe *dp, const char *src, const char *dst,
                EchoMode mode) {
  int in_fd = -1;
  int out_fd = -1;
  int rc = -1;
  struct stat st;
  struct timespec t0, t1;
  double seconds;

  in_fd = open(src, O_RDONLY);
  if (in_fd < 0) {
    perror(src);
    goto cleanup;
  }
  out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (out_fd < 0) {
    perror(dst);
    goto cleanup;
  }
  if (fstat(in_fd, &st) < 0) {
    perror("fstat");
    goto cleanup;
  }

  (void)posix_fadvise(in_fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  (void)posix_fadvise(out_fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  (void)fallocate(out_fd, 0, 0, st.st_size);

  fprintf(stderr, "mode %s\n", mode == ECHO_MODE_SPLICE ? "splice" : "copy");
  fprintf(stderr, "pipe_capacity %d\n", dp->capacity);
  fprintf(stderr, "file_bytes %lld\n", (long long)st.st_size);

  if (dp->capacity <= 0) {
    fprintf(stderr, "pipe capacity is empty\n");
    goto cleanup;
  }

  if (clock_gettime(CLOCK_MONOTONIC, &t0) < 0) {
    perror("clock_gettime");
    goto cleanup;
  }
  if (transfer(dp, in_fd, out_fd, st.st_size, mode) < 0)
    goto cleanup;
  if (clock_gettime(CLOCK_MONOTONIC, &t1) < 0) {
    perror("clock_gettime");
    goto cleanup;
  }

  seconds =
      (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
  printf("transfer_s %.6f\n", seconds);
  if (seconds > 0.0) {
    double mib = (double)st.st_size / (1024.0 * 1024.0);
    printf("file_mib_per_s %.1f\n", mib / seconds);
  }
  rc = 0;

cleanup:
  if (in_fd >= 0)
    close(in_fd);
  if (out_fd >= 0)
    close(out_fd);
  return rc;
}
