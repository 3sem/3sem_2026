#include "executor.h"
#include "parser.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int verbose_fd = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose_fd = 1;
        }
    }

    /* Ignore Ctrl+C */
    signal(SIGINT, SIG_IGN);

    char *line = NULL;
    size_t line_cap = 0;
    int interactive = isatty(STDIN_FILENO);

    while (1) {
        if (interactive) {
            fprintf(stderr, "mysh> ");
            fflush(stderr);
        }

        errno = 0;
        ssize_t nread = getline(&line, &line_cap, stdin);
        if (nread == -1) {
            if (errno == EINTR) {
                clearerr(stdin);
                if (interactive) {
                    fprintf(stderr, "\n");
                }
                continue;
            }
            if (interactive) {
                fprintf(stderr, "\n");
            }
            break;
        }
        if (nread > 0 && line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        size_t num_cmds = 0;
        char ***cmds = parse_pipeline(line, &num_cmds);
        if (!cmds || num_cmds == 0) {
            continue;
        }

        int builtin_status = handle_builtin(cmds, num_cmds);
        if (builtin_status == -1) {
            free_pipeline(cmds);
            break;
        } else if (builtin_status == 1) {
            free_pipeline(cmds);
            continue;
        }

        execute_pipeline(cmds, num_cmds, verbose_fd);
        free_pipeline(cmds);
    }

    free(line);
    return 0;
}