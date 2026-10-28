#include "buffer.h"

buffer_t *create_buffer(){
    buffer_t *buf   = malloc (sizeof(buffer_t));
    if (!buf){
        perror("buf memory error");
        return NULL;
    }

    buf->capacity = MAX_BUF_SIZE;
    buf->size     = 0;
    buf->actions  = &default_buf_ops;

    return buf;
}

void destroy_buffer(buffer_t *buf){
    free(buf);

    return;
}

ssize_t write_in_buffer(buffer_t *buffer, int input_fd){
    if (!buffer || input_fd < 0){
        return -1;
    }

    buffer->size = read(input_fd, buffer->buffer, buffer->capacity);

    return buffer->size;
}

ssize_t read_from_buffer(buffer_t *buffer, int output_fd){
    if (!buffer || output_fd < 0){
        return -1;
    }

    buffer->size = write(output_fd, buffer->buffer, buffer->size);

    return buffer->size;
}