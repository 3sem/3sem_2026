#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <glob.h>

int main() {
    while (1) {
        printf("my_shell> ");
        fflush(stdout);

        char* buffer = NULL;
        size_t size = 0;

        ssize_t chars_read = getline(&buffer, &size, stdin);
        if (chars_read == -1) {
            printf("\n");
            break;
        }
        if (buffer[chars_read - 1] == '\n') {
            buffer[chars_read - 1] = '\0';
        }

        char* cmds[256] = {0};
        int cmd_count = 0;
        char* save_ptr_pipe = NULL;

        char* cmd = strtok_r(buffer, "|", &save_ptr_pipe);
        while (cmd != NULL) {
            if (cmd_count == 256) {
                fprintf(stderr, "too many commands\n");
                break;
            }

            cmds[cmd_count] = cmd;
            cmd_count++;

            cmd = strtok_r(NULL, "|", &save_ptr_pipe);
        }

        pid_t pids[256] = {0};
        int launched = 0;
        int in_fd = 0;

        for (int i = 0; i < cmd_count; i++) {
            char* args[256] = {0};
            int arg_count = 0;
            char* save_ptr_arg = NULL;

            char *token = strtok_r(cmds[i], " \t", &save_ptr_arg);
            while (token != NULL) {
                if (arg_count == 255) {
                    fprintf(stderr, "too many arguments\n");
                    break;
                }

                args[arg_count] = token;
                arg_count++;
                token = strtok_r(NULL, " \t", &save_ptr_arg);
            }
            args[arg_count] = NULL;

            if (arg_count == 0) { //if user writes " | | "
                continue;
            }

            int is_last = 0;
            if (i == cmd_count - 1) {
                is_last = 1;
            }
            else {
                is_last = 0;
            }

            int fd[2] = {0};

            if (is_last == 0) {
                if (pipe(fd) < 0) {
                    perror("Ошибка pipe");
                    exit(1);
                }
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork error");
                exit(1);
            }

            if (pid == 0) { //child process
                if (in_fd != 0) {
                    dup2(in_fd, 0);
                    close(in_fd);
                }

                if (is_last == 0) {
                    dup2(fd[1], 1);
                    close(fd[1]);
                    close(fd[0]);
                }

                glob_t g;
                for (int j = 0; j < arg_count; j++) {
                    int flags = GLOB_NOCHECK;
                    if (j > 0) {
                        flags |= GLOB_APPEND;
                    }
                    glob(args[j], flags, NULL, &g);
                }
                execvp(g.gl_pathv[0], g.gl_pathv);

                perror("execvp error");
                exit(1);
            }
            else { //parent process
                pids[launched] = pid;
                launched++;

                if (in_fd != 0) {
                    close(in_fd);
                }

                if (!is_last) {
                    close(fd[1]);
                    in_fd = fd[0];
                }
            }

        }

        for (int i = 0; i < launched; i++) {
            int status = 0;
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status)) {
                printf("exit code: %d\n", WEXITSTATUS(status));
            }
            else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
            }
        }

        free(buffer);
    }
}
