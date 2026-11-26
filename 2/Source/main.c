#include "include.h"

int main() {
    pid_t pid;
    int status;
    buffer_t    *buffer;
    duplex_ch_t *duplex_ch;

    if (!(buffer    = create_buffer ())) return 0;
    if (!(duplex_ch = create_channel())) return 0;

    pid = fork();

    switch (pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;

        case 0: 
            start_child(duplex_ch, buffer);
            break;

        default:
            start_parent(duplex_ch, buffer);
            waitpid(pid, &status, 0);
            break;
    }

    destroy_buffer (buffer);
    destroy_channel(duplex_ch);
    
    return 0;
}

void start_parent(duplex_ch_t *duplex_ch, buffer_t *buffer){
    duplex_ch->actions->close_unused_parent(duplex_ch);

    sleep(1);
    int output_fd = open(destination, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (output_fd == -1) {
        perror("open source error");
        duplex_ch->actions->close_parent(duplex_ch);
        return;
    }

    while ((buffer->actions->read_in_buffer(buffer, duplex_ch->pipe_back[0])) > 0) {
        buffer->actions->wright_from_buffer(buffer, output_fd);
    }

    close(output_fd);
    duplex_ch->actions->close_parent(duplex_ch);
}

void start_child(duplex_ch_t *duplex_ch, buffer_t *buffer){
    duplex_ch->actions->close_unused_child(duplex_ch);

    int input_fd = open(source, O_RDONLY);
    if (input_fd == -1) {
        perror("open source error");
        duplex_ch->actions->close_child(duplex_ch);
        return;
    }

    while ((buffer->actions->read_in_buffer(buffer, input_fd)) > 0){
        buffer->actions->wright_from_buffer(buffer, duplex_ch->pipe_back[1]);
    }

    close(input_fd);
    duplex_ch->actions->close_child(duplex_ch);
}