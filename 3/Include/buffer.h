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

static const char source[]      = "Source.txt";
static const char destination[] = "Destination.txt";

//====================buffer=========================
#if defined(SYS_V_QUEUE)
#define MAX_SYS_V_SIZE 8192
#define MAX_BUF_SIZE   MAX_SYS_V_SIZE

#elif defined(FIFO)
#define MAX_BUF_SIZE   65536

#elif defined(SYS_V)
#define MAX_BUF_SIZE   16777216
#endif

typedef struct Buffer buffer_t;

typedef struct {
    ssize_t (*write_in_buffer )(buffer_t *self, int fd);
    ssize_t (*read_from_buffer)(buffer_t *self, int fd);
} BufferOps;

#if defined(SYS_V_QUEUE)

struct Buffer{
    long   mtype;
    char   buffer[MAX_BUF_SIZE];
    size_t size;
    size_t capacity;
    const BufferOps *actions;
};

//#elif defined(FIFO)
#else
struct Buffer{ //fifo
    char   buffer[MAX_BUF_SIZE];
    size_t size;
    size_t capacity;
    const BufferOps *actions;
};
#endif

ssize_t write_in_buffer  (buffer_t *buffer, int input_fd);
ssize_t read_from_buffer (buffer_t *buffer, int output_fd);

static const BufferOps default_buf_ops = {
    .read_from_buffer = read_from_buffer,
    .write_in_buffer  = write_in_buffer,
};

buffer_t *create_buffer  ();
void      destroy_buffer(buffer_t *buf);