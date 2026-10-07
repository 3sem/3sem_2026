#ifdef SYS_V_QUEUE

#include "sys_v_queue.h"

void start_parent(sys_v_queue_t *sys_v_queue, buffer_t *buffer){
    int output_fd = open(destination, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (output_fd == -1) {
        perror("open source error");
        return;
    }

    while (!sys_v_queue->sys_v_queue_ops.read_from_sys_v_queue(sys_v_queue, buffer)) {
        if (buffer->mtype == 666) break;
 
        buffer->actions->read_from_buffer(buffer, output_fd);
    }

    close(output_fd);
}

void start_child(sys_v_queue_t *sys_v_queue, buffer_t *buffer){
    int input_fd = open(source, O_RDONLY);
    if (input_fd == -1) {
        perror("open source error");
        return;
    }

    while ((buffer->actions->write_in_buffer(buffer, input_fd)) > 0){
        sys_v_queue->sys_v_queue_ops.write_in_sys_v_queue(sys_v_queue, buffer, 1);
    }

    buffer->size  = 0;
    sys_v_queue->sys_v_queue_ops.write_in_sys_v_queue(sys_v_queue, buffer, 666);

    close(input_fd);
}

sys_v_queue_t *create_sys_v_queue(){
    sys_v_queue_t* sys_v_queue = malloc(sizeof(sys_v_queue_t));
    
    if (!sys_v_queue){
        perror("Memory error");
        return NULL;    
    }

    sys_v_queue->key    = ftok("/", 'M');
    sys_v_queue->msgflg = IPC_CREAT | 0666;
    sys_v_queue->max_size = MAX_SYS_V_SIZE;
    if ((sys_v_queue->msqid = msgget(sys_v_queue->key, sys_v_queue->msgflg )) < 0) {
        perror("msgget");
        exit(1);
    }
    sys_v_queue->sys_v_queue_ops = default_sys_v_queue_ops;
    return sys_v_queue;
}

void destroy_sys_v_queue(sys_v_queue_t* sys_v_queue){
    free(sys_v_queue);
}

int write_in_sys_v_queue(sys_v_queue_t* self, buffer_t *buffer, int mtype){
    buffer->mtype = mtype;

    if (msgsnd(self->msqid, buffer, buffer->size, 0) < 0) {
        perror("msgsnd");
        return 1;
    }

    return 0;
}

int read_from_sys_v_queue(sys_v_queue_t* self, buffer_t *buffer){
    if ((buffer->size = msgrcv(self->msqid, buffer, buffer->capacity, 0, 0)) < 0){
        perror("msgrcv");
        return 1;
    }

    return 0;
}

#endif