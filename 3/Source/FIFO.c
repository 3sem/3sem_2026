#ifdef FIFO

#include "FIFO.h"

void start_parent(ch_t *ch, buffer_t *buffer){
    ch->actions->open_read_ch(ch);

    int output_fd = open(destination, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (output_fd == -1) {
        perror("open source error");
        return;
    }

    while ((buffer->actions->write_in_buffer(buffer, ch->ch)) > 0) {
        buffer->actions->read_from_buffer(buffer, output_fd);
    }

    close(output_fd);
}

void start_child(ch_t *ch, buffer_t *buffer){
    ch->actions->open_write_ch(ch);

    int input_fd = open(source, O_RDONLY);
    if (input_fd == -1) {
        perror("open source error");
        return;
    }

    while ((buffer->actions->write_in_buffer(buffer, input_fd)) > 0){
        buffer->actions->read_from_buffer(buffer, ch->ch);
    }

    close(input_fd);
}

ch_t *create_channel (const char *fifo_path){
    if (!fifo_path){
        perror("fifo path error");
        return NULL;
    }
    unlink(fifo_path);
    
    if (mkfifo(fifo_path, 0666) == -1) {
        perror("mkfifo error");
        return NULL;
    }

    ch_t* ch = malloc(sizeof(ch_t));
    
    if (!ch){
        perror("Memory error");
        return NULL;    
    }

    ch->fifo_path = fifo_path;
    ch->actions   = &default_channel_ops;

    return ch;
}

void destroy_channel(ch_t *ch){
    if (ch->ch != -1)        close (ch->ch);
    if (ch->fifo_path) unlink(ch->fifo_path);
    
    free(ch);
}

void open_read_ch(ch_t *ch){
    ch->ch = open(ch->fifo_path, O_RDONLY);
}

void open_write_ch(ch_t *ch){
    ch->ch = open(ch->fifo_path, O_WRONLY);
}

#endif