#include "include.h"

int main() {
    pid_t pid;
    int status;
    buffer_t      *buffer;
    if (!(buffer      = create_buffer     ())) return 0;

#if defined(SYS_V_QUEUE)
    sys_v_queue_t *sys_v_queue;
    if (!(sys_v_queue = create_sys_v_queue())) return 0;
#elif defined(FIFO)
    ch_t *ch;
    if (!(ch = create_channel(default_fifo_path))) return 0;
#endif

    pid = fork();

    switch (pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0:
            fprintf(stderr, "child start\n");
#if defined(SYS_V_QUEUE)
            start_child(sys_v_queue, buffer);
#elif defined(FIFO)
            start_child(ch, buffer);
#endif
            fprintf(stderr, "child end\n");
            break;

        default:
            fprintf(stderr, "parent start\n");
#if defined(SYS_V_QUEUE)
            start_parent(sys_v_queue, buffer);
#elif defined(FIFO)
            start_parent(ch, buffer);
#endif
            waitpid(pid, &status, 0);
            fprintf(stderr, "parent end\n");
            break;
    }

    destroy_buffer(buffer);

#if defined(SYS_V_QUEUE)
    destroy_sys_v_queue(sys_v_queue);
#elif defined(FIFO)
    destroy_channel(ch);
#endif
}