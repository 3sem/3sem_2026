#ifndef SYS_V_QUEUE_H_
#define SYS_V_QUEUE_H_ 

#include <buffer.h>

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
