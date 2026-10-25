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

ssize_t put_num_in_buf(buffer_t *buffer, int num){
    if (!buffer || buffer->size == buffer->capacity){
        return -1;
    }

    buffer->buffer[buffer->size] = num;
    buffer->size++;

    return buffer->size;
}