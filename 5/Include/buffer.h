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

static const char source[]      = "Source.txt";
static const char destination[] = "Destination.txt";

#define MAX_BUF_SIZE 1024

typedef struct Buffer buffer_t;

typedef struct {
    ssize_t (*read_from_buffer)(buffer_t *self, int fd);
    ssize_t (*write_in_buffer )(buffer_t *self, int fd);
} BufferOps;

struct Buffer{
    char   buffer[MAX_BUF_SIZE];
    size_t size;
    size_t max_size;
    const BufferOps *actions;
};

ssize_t write_in_buffer  (buffer_t *buffer, int input_fd);
ssize_t read_from_buffer (buffer_t *buffer, int output_fd);

static const BufferOps default_buf_ops = {
    .read_from_buffer = read_from_buffer,
    .write_in_buffer  = write_in_buffer,
};

buffer_t *create_buffer ();
void      destroy_buffer(buffer_t *buf);