#include "include.h"

static void run_child  (int  fd_in, int p[2], int i, char ***cmd);
static void run_parent (int *fd_in, int p[2], int i);

void seq_pipeline(char ***cmd){
    int p[2];
    pid_t pid;

    int fd_in = 0;
    int i     = 0;

    while (cmd[i] != NULL){
        pipe(p);

        switch (pid = fork()){
            case -1:  perror("fork error");
            case  0:  run_child ( fd_in, p, i, cmd);
            default:  run_parent(&fd_in, p, i);
        }
        i++;
    }

    return;
}

void run_child(int fd_in, int p[2], int i, char ***cmd){
    if (i > 0){
        dup2(fd_in, STDIN_FILENO);
    }
    if (cmd[i+1] != NULL){
        dup2(p[1], STDOUT_FILENO);
    }
    close(p[0]);

    execvp(cmd[i][0], cmd[i]);
    exit(2);
}

void run_parent(int *fd_in, int p[2], int i){
    //parent
    close(p[1]);
    if (i > 0){
        close(*fd_in);
    }

    *fd_in = p[0]; //TODO check if }

    wait(NULL);
    i++;
}
