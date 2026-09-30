#include "include.h"

duplex_ch_t *create_channel(){
    duplex_ch_t* duplex_ch = malloc(sizeof(duplex_ch_t));
    
    if (!duplex_ch){
        perror("Memory error");
        return NULL;    
    }

    if (pipe(duplex_ch->pipe_direct)){
        perror("Pipe direct error");
        free(duplex_ch);

        return NULL;
    }
    
    if (pipe(duplex_ch->pipe_back)){
        perror("Pipe back error");
        close(duplex_ch->pipe_direct[0]);
        close(duplex_ch->pipe_direct[1]);
        free(duplex_ch);

        return NULL;
    }

    duplex_ch->actions = &default_channel_ops;

    return duplex_ch;
}

void destroy_channel(duplex_ch_t *duplex_ch){
    free(duplex_ch);
}

void close_unused_parent(duplex_ch_t *self){
    close(self->pipe_direct[0]);
    close(self->pipe_back  [1]);
}

void close_unused_child(duplex_ch_t *self){
    close(self->pipe_direct[1]);
    close(self->pipe_back  [0]);
}

void close_parent(duplex_ch_t *self){
    close(self->pipe_direct[1]);
    close(self->pipe_back  [0]);
}

void close_child(duplex_ch_t *self){
    close(self->pipe_direct[0]);
    close(self->pipe_back  [1]);
}
