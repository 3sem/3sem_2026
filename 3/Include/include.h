#ifndef INCLUDE_H_
#define INCLUDE_H_

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h> 
#include <sys/msg.h>  
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <stdio.h>
#include <unistd.h>
/*
TODO
mmap вместо буфера!!!!!
*/
#define FIFO

#if defined(SYS_V_QUEUE)
#define MAX_SYS_V_SIZE 8192
#define MAX_BUF_SIZE   MAX_SYS_V_SIZE

#elif defined(FIFO)
#define MAX_BUF_SIZE   65536
#endif

static const char source[]      = "Source.txt";
static const char destination[] = "Destination.txt";

//====================buffer=========================
typedef struct Buffer buffer_t;

typedef struct {
    ssize_t (*write_in_buffer )(buffer_t *self, int fd);
    ssize_t (*read_from_buffer)(buffer_t *self, int fd);
} BufferOps;

struct Buffer{
    long   mtype;
    char   buffer[MAX_BUF_SIZE];
    size_t size;
    size_t capacity;
    const BufferOps *actions;
};

ssize_t write_in_buffer  (buffer_t *buffer, int input_fd);
ssize_t read_from_buffer (buffer_t *buffer, int output_fd);

static const BufferOps default_buf_ops = {
    .read_from_buffer = read_from_buffer,
    .write_in_buffer  = write_in_buffer,
};

buffer_t *create_buffer  ();
void      destroy_buffer(buffer_t *buf);

//=================sys_v_queue===================
#ifdef SYS_V_QUEUE
typedef struct Sys_v_queue sys_v_queue_t;

typedef struct{
    int (*write_in_sys_v_queue )(sys_v_queue_t*, buffer_t*, int);
    int (*read_from_sys_v_queue)(sys_v_queue_t*, buffer_t*);
} sys_v_queue_ops;

struct Sys_v_queue{
    int msqid;
    int key;
    int msgflg;
    size_t max_size;
    sys_v_queue_ops sys_v_queue_ops;
};

int write_in_sys_v_queue(sys_v_queue_t* self, buffer_t *buffer, int mtype);
int read_from_sys_v_queue(sys_v_queue_t* self, buffer_t *buffer);

static sys_v_queue_ops default_sys_v_queue_ops = {
    .write_in_sys_v_queue  = write_in_sys_v_queue,
    .read_from_sys_v_queue = read_from_sys_v_queue,
};

sys_v_queue_t *create_sys_v_queue();
void destroy_sys_v_queue(sys_v_queue_t *sys_v_queue);

void start_parent(sys_v_queue_t *sys_v_queue, buffer_t *buffer);
void start_child (sys_v_queue_t *sys_v_queue, buffer_t *buffer);
#endif

//===========================FIFO=======================
#ifdef FIFO

static const char default_fifo_path[] = "/tmp/my_fifo";

typedef struct Channel ch_t;

typedef struct {
    void (*open_read_ch ) (ch_t *ch);
    void (*open_write_ch) (ch_t *ch);
} ChannelOps;

struct Channel {
    int         ch;
    const char *fifo_path;
    const ChannelOps *actions;
};

void open_read_ch (ch_t *ch);
void open_write_ch(ch_t *ch);

static const ChannelOps default_channel_ops = {
    .open_read_ch  = open_read_ch,
    .open_write_ch = open_write_ch,
};

ch_t *create_channel (const char *fifo_path);
void  destroy_channel(ch_t *ch);

void start_parent(ch_t *duplex_ch, buffer_t *buffer);
void start_child (ch_t *duplex_ch, buffer_t *buffer);
#endif

#endif