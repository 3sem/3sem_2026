#ifndef FIFO_H_
#define FIFO_H_

#include <buffer.h>

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


void start_parent(ch_t *duplex_ch, buffer_t *buffer); //fifo
void start_child (ch_t *duplex_ch, buffer_t *buffer);

#endif