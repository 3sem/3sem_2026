#include "executor.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int handle_builtin(char ***cmds, size_t num_cmds) {
    if (num_cmds != 1 || !cmds[0] || !cmds[0][0]) {
        return 0;
    }

    const char *cmd = cmds[0][0];
    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        return -1;
    }

    if (strcmp(cmd, "cd") == 0) {
        const char *target_dir = cmds[0][1];
        if (!target_dir) {
            target_dir = getenv("HOME");
            if (!target_dir) {
                fprintf(stderr, "cd: переменная HOME не задана\n");
                return 1;
            }
        }
        if (chdir(target_dir) != 0) {
            perror("cd");
        }
        return 1;
    }

    return 0;
}

void execute_pipeline(char ***cmd, size_t num_cmds, int verbose_fd) {
    pid_t *pids = malloc(num_cmds * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        return;
    }

    int fd_in = STDIN_FILENO;

    for (size_t i = 0; i < num_cmds; ++i) {
        int p[2] = {-1, -1};
        int has_next = (cmd[i + 1] != NULL);

        if (has_next) {
            if (pipe(p) == -1) {
                perror("pipe");
                if (fd_in != STDIN_FILENO) {
                    close(fd_in);
                }
                num_cmds = i;
                break;
            }
            if (verbose_fd) {
                fprintf(stderr, "New used pipe descriptors: %d %d\n", p[0], p[1]);
            }
        }

        if (verbose_fd) {
            fprintf(stderr, "Input descriptor for current child process: %d\n", fd_in);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (has_next) {
                close(p[0]);
                close(p[1]);
            }
            if (fd_in != STDIN_FILENO) {
                close(fd_in);
            }
            num_cmds = i;
            break;
        } else if (pid == 0) {
            signal(SIGINT, SIG_DFL);

            if (fd_in != STDIN_FILENO) {
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2 (stdin)");
                    _exit(EXIT_FAILURE);
                }
                close(fd_in);
            }

            if (has_next) {
                close(p[0]); /* Читающий конец нового канала ребёнку не нужен */
                if (dup2(p[1], STDOUT_FILENO) == -1) {
                    perror("dup2 (stdout)");
                    close(p[1]);
                    _exit(EXIT_FAILURE);
                }
                close(p[1]);
            }

            execvp(cmd[i][0], cmd[i]);
            fprintf(stderr, "mysh: %s: %s\n", cmd[i][0], strerror(errno));
            _exit(127);
        } else {
            pids[i] = pid;

            if (fd_in != STDIN_FILENO) {
                close(fd_in);
            }

            if (has_next) {
                close(p[1]);
                fd_in = p[0];
            }
        }
    }

    for (size_t i = 0; i < num_cmds; ++i) {
        int status = 0;
        pid_t wpid;
        do {
            wpid = waitpid(pids[i], &status, 0);
        } while (wpid == -1 && errno == EINTR);

        if (wpid == -1) {
            perror("waitpid");
            continue;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            fprintf(stderr, "[pid %d (%s) завершился с кодом: %d]\n",
                    (int)pids[i], cmd[i][0], exit_code);
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            fprintf(stderr, "[pid %d (%s) убит сигналом: %d (%s)]\n",
                    (int)pids[i], cmd[i][0], sig, strsignal(sig));
        }
    }

    free(pids);
}