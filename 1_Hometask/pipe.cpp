#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "pipe.h"

void printProcessStatus(pid_t pid, const char *command, int status) {
    if (WIFEXITED(status)) {
        fprintf(stderr, "[%d] %s: exit code %d\n", pid, command, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "[%d] %s: signal %d\n", pid, command, WTERMSIG(status));
    }
}

void seqPipe(char ***cmd) {
    int p[2] = {};
    pid_t pid = 0;
    int fd_in = STDIN_FILENO;
    int i = 0;

    while (cmd[i] != NULL) {
        if (pipe(p) == -1) {
            perror("pipe");

            if (i > 0)
                close(fd_in);

            return;
        }

        if ((pid = fork()) == -1) {
            perror("fork");

            close(p[0]);
            close(p[1]);

            if (i > 0)
                close(fd_in);

            return;
        }

        if (pid == 0) {
            if (i > 0) {
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2 stdin");
                    _exit(126);
                }
                close(fd_in);
            }

            if (cmd[i + 1] != NULL) {
                if (dup2(p[1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout");
                    _exit(126);
                }
            }

            close(p[0]);
            close(p[1]);

            execvp(cmd[i][0], cmd[i]);

            fprintf(stderr, "%s: %s\n", cmd[i][0], strerror(errno));
            _exit(errno == ENOENT ? 127 : 126);
        }

        int status = 0;
        pid_t waitResult = 0;

        do {
            waitResult = waitpid(pid, &status, 0);
        } while (waitResult == -1 && errno == EINTR);

        if (waitResult == -1) {
            perror("waitpid");

            close(p[0]);
            close(p[1]);

            if (i > 0) {
                close(fd_in);
            }

            return;
        }

        printProcessStatus(pid, cmd[i][0], status);

        close(p[1]);

        if (i > 0) {
            close(fd_in);
        }
        fd_in = p[0];

        ++i;
    }

    if (i > 0) {
        close(fd_in);
    }
}
