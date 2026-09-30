#ifndef INCLUDE_H_
#define INCLUDE_H_

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE 65536

static const char source[]      = "Source.txt";
static const char destination[] = "Destination.txt";

//====================buffer=========================
typedef struct Buffer buffer_t;

typedef struct {
    ssize_t (*wright_from_buffer)(buffer_t *self, int fd);
    ssize_t (*read_in_buffer )(buffer_t *self, int fd);
} BufferOps;

struct Buffer{
    ssize_t  size;
    size_t  capacity;
    char    buffer[BUF_SIZE];
    const BufferOps *actions;
};

ssize_t read_in_buffer    (buffer_t *buffer, int input_fd);
ssize_t wright_from_buffer(buffer_t *buffer, int output_fd);

static const BufferOps default_buf_ops = {
    .read_in_buffer = read_in_buffer,
    .wright_from_buffer  = wright_from_buffer
};

buffer_t *create_buffer  ();
void      destroy_cmd_arr(buffer_t *buf);

//=================DuplexChannel===================
typedef struct DuplexChannel duplex_ch_t;

typedef struct {
    void (*close_unused_parent)(duplex_ch_t *self);
    void (*close_parent)       (duplex_ch_t *self);
    void (*close_unused_child )(duplex_ch_t *self);
    void (*close_child )       (duplex_ch_t *self);
} ChannelOps;

struct DuplexChannel {
    int pipe_direct[2];
    int pipe_back[2]; 
    const ChannelOps *actions;
};

duplex_ch_t *create_channel();
void         destroy_channel(duplex_ch_t *ch);

void close_unused_parent(duplex_ch_t *self);
void close_unused_child(duplex_ch_t *self);
void close_parent(duplex_ch_t *self);
void close_child(duplex_ch_t *self);

static const ChannelOps default_channel_ops = {
    .close_unused_parent = close_unused_parent,
    .close_unused_child  = close_unused_child,
    .close_parent        = close_parent,
    .close_child         = close_child,
};

void start_parent(duplex_ch_t *duplex_ch, buffer_t *buffer);
void start_child (duplex_ch_t *duplex_ch, buffer_t *buffer);

#endif