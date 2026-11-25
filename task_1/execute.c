#define _POSIX_C_SOURCE 200809L
#include "execute.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_commands(const Pipeline *p)
{
    pid_t *pids = calloc((size_t)p->count, sizeof(pid_t));
    if (pids == NULL) {
        perror("calloc");
        return 1;
    }

    int prev = -1;
    int started = 0;
    int error = 0;

    for (int i = 0; i < p->count; i++) {
        int fd[2] = {-1, -1};

        if (i + 1 < p->count && pipe(fd) == -1) {
            perror("pipe");
            error = 1;
            break;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (prev != -1)
                close(prev);
            if (fd[0] != -1)
                close(fd[0]);
            if (fd[1] != -1)
                close(fd[1]);
            error = 1;
            break;
        }

        if (pid == 0) {
            if (prev != -1)
                dup2(prev, STDIN_FILENO);
            if (fd[1] != -1)
                dup2(fd[1], STDOUT_FILENO);

            if (prev != -1)
                close(prev);
            if (fd[0] != -1)
                close(fd[0]);
            if (fd[1] != -1)
                close(fd[1]);

            execvp(p->commands[i].argv[0], p->commands[i].argv);
            perror(p->commands[i].argv[0]);
            _exit(127);
        }

        pids[started++] = pid;

        if (prev != -1)
            close(prev);
        if (fd[1] != -1)
            close(fd[1]);
        prev = fd[0];
    }

    if (prev != -1)
        close(prev);

    int last = 1;
    for (int i = 0; i < started; i++) {
        int status;
        pid_t result;

        do {
            result = waitpid(pids[i], &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            error = 1;
        } else if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            printf("process %ld finished with code %d\n", (long)pids[i], code);
            if (i == p->count - 1)
                last = code;
        } else if (WIFSIGNALED(status)) {
            int code = 128 + WTERMSIG(status);
            printf("process %ld stopped by signal, code %d\n", (long)pids[i], code);
            if (i == p->count - 1)
                last = code;
        }
    }

    free(pids);
    if (error)
        return 1;
    return last;
}
