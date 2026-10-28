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

#define KB * (1 << 10)

static const char source[]      = "Source.txt";
static const char destination[] = "Destination.txt";


#define MAX_BUF_SIZE   (64 KB)

typedef struct Buffer buffer_t;

typedef struct {
    ssize_t (*put_num_in_buf )(buffer_t *self, int num, int id);
} BufferOps;

struct Buffer{
    int    buffer[MAX_BUF_SIZE];
    size_t size;
    size_t capacity;
    const BufferOps *actions;
};

ssize_t put_num_in_buf (buffer_t *buffer, int output_fd, int id);

static const BufferOps default_buf_ops = {
    .put_num_in_buf = put_num_in_buf,
};

buffer_t *create_buffer  ();
void      destroy_buffer(buffer_t *buf);